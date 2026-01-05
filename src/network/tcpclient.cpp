#include "tcpclient.h"
#include <QDebug>
#include <QSslCipher>
#include <QSslConfiguration>
#include <QSslPreSharedKeyAuthenticator>
#include <QSslSocket>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent), m_socket(new QSslSocket(this)), m_protocol(new Protocol(this)), m_authTimer(new QTimer(this)),
      m_pingTimer(new QTimer(this)), m_port(K4Protocol::DEFAULT_PORT), m_useTls(false), m_state(Disconnected),
      m_authResponseReceived(false) {
    // Socket signals
    connect(m_socket, &QSslSocket::connected, this, &TcpClient::onSocketConnected);
    connect(m_socket, &QSslSocket::encrypted, this, &TcpClient::onSocketEncrypted);
    connect(m_socket, &QSslSocket::disconnected, this, &TcpClient::onSocketDisconnected);
    connect(m_socket, &QSslSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, &QSslSocket::errorOccurred, this, &TcpClient::onSocketError);

    // SSL-specific signals
    connect(m_socket, &QSslSocket::sslErrors, this, &TcpClient::onSslErrors);
    connect(m_socket, &QSslSocket::preSharedKeyAuthenticationRequired, this,
            &TcpClient::onPreSharedKeyAuthenticationRequired);

    // Auth timeout timer (single shot)
    m_authTimer->setSingleShot(true);
    connect(m_authTimer, &QTimer::timeout, this, &TcpClient::onAuthTimeout);

    // Ping timer for keep-alive
    m_pingTimer->setInterval(K4Protocol::PING_INTERVAL_MS);
    connect(m_pingTimer, &QTimer::timeout, this, &TcpClient::onPingTimer);

    // Protocol signals - any packet means auth succeeded
    connect(m_protocol, &Protocol::packetReceived, this, [this](quint8 type, const QByteArray &payload) {
        Q_UNUSED(payload)
        if (m_state == Authenticating && !m_authResponseReceived) {
            m_authResponseReceived = true;
            m_authTimer->stop();
            qDebug() << "Authentication successful, received packet type:" << type;
            setState(Connected);
            emit authenticated();
            startPingTimer();

            // Send initialization sequence
            // RDY triggers comprehensive state dump containing all radio state:
            // FA, FB, MD, MD$, BW, BW$, IS, CW, KS, PC, SD (per mode), SQ, RG, SQ$, RG$,
            // #SPN, #REF, VXC, VXV, VXD, and all menu definitions (MEDF)
            qDebug() << "Sending initialization commands...";
            sendCAT("RDY;");   // Ready - triggers comprehensive state dump (also enables AI mode)
            sendCAT("K41;");   // Enable advanced K4 protocol mode
            sendCAT("ER1;");   // Request long format error messages
            sendCAT("EM3;");   // Set audio to Opus Float 32-bit mode (required for audio streaming)
        }
    });
    connect(m_protocol, &Protocol::catResponseReceived, this, &TcpClient::onCatResponse);
}

TcpClient::~TcpClient() {
    stopPingTimer();
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
}

void TcpClient::connectToHost(const QString &host, quint16 port, const QString &password, bool useTls,
                              const QString &psk) {
    if (m_state != Disconnected) {
        disconnectFromHost();
    }

    m_host = host;
    m_port = port;
    m_password = password;
    m_useTls = useTls;
    m_psk = psk;
    m_authResponseReceived = false;

    setState(Connecting);

    if (m_useTls) {
        // Log OpenSSL version Qt is using
        qDebug() << "=== SSL Library Info ===";
        qDebug() << "  Build version:" << QSslSocket::sslLibraryBuildVersionString();
        qDebug() << "  Runtime version:" << QSslSocket::sslLibraryVersionString();
        qDebug() << "  Supports SSL:" << QSslSocket::supportsSsl();

        // Configure TLS with PSK cipher suites
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setProtocol(QSsl::TlsV1_2);
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone); // PSK doesn't use certificates

        // PSK cipher suite - only ECDHE-PSK-CHACHA20-POLY1305 gives us TLSv1.2 + forward secrecy
        // K4 server prefers weaker ciphers, so we only offer the best one
        // Other PSK ciphers (even ECDHE variants) fall back to TLSv1.0 in Qt's OpenSSL
        QList<QSslCipher> pskCiphers;
        const QStringList pskCipherNames = {"ECDHE-PSK-CHACHA20-POLY1305"};

        // Add ciphers in our preference order (not system order)
        QList<QSslCipher> allCiphers = QSslConfiguration::supportedCiphers();
        for (const QString &name : pskCipherNames) {
            for (const QSslCipher &cipher : allCiphers) {
                if (cipher.name() == name) {
                    pskCiphers.append(cipher);
                    qDebug() << "Added PSK cipher:" << cipher.name() << "protocol:" << cipher.protocolString();
                    break;
                }
            }
        }
        qDebug() << "Configured" << pskCiphers.size() << "PSK ciphers, first:"
                 << (pskCiphers.isEmpty() ? "none" : pskCiphers.first().name());

        if (pskCiphers.isEmpty()) {
            qWarning() << "No PSK ciphers available! TLS/PSK connection will likely fail.";
            qWarning() << "Ensure OpenSSL 3.x is installed (brew install openssl@3)";
            emit errorOccurred("TLS/PSK not available: No PSK cipher suites found. "
                               "Install OpenSSL 3.x or use unencrypted connection (port 9205).");
            setState(Disconnected);
            return;
        }
        sslConfig.setCiphers(pskCiphers);

        m_socket->setSslConfiguration(sslConfig);

        qDebug() << "Connecting with TLS/PSK to" << host << ":" << port;
        m_socket->connectToHostEncrypted(host, port);
    } else {
        qDebug() << "Connecting (unencrypted) to" << host << ":" << port;
        m_socket->connectToHost(host, port);
    }
}

void TcpClient::disconnectFromHost() {
    stopPingTimer();
    m_authTimer->stop();

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        // Send graceful disconnect command
        if (m_state == Connected) {
            sendCAT("RRN;");
        }
        m_socket->disconnectFromHost();
    }

    setState(Disconnected);
}

bool TcpClient::isConnected() const {
    return m_state == Connected;
}

TcpClient::ConnectionState TcpClient::connectionState() const {
    return m_state;
}

void TcpClient::sendCAT(const QString &command) {
    if (m_state == Connected) {
        QByteArray packet = Protocol::buildCATPacket(command);
        m_socket->write(packet);
    }
}

void TcpClient::sendRaw(const QByteArray &data) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(data);
    }
}

void TcpClient::setState(ConnectionState state) {
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);

        if (state == Connected) {
            emit connected();
        } else if (state == Disconnected) {
            emit disconnected();
        }
    }
}

void TcpClient::onSocketConnected() {
    if (m_useTls) {
        // TLS connection: TCP connected, now waiting for TLS handshake to complete
        // The encrypted() signal will fire when TLS is fully established
        qDebug() << "TCP connected, starting TLS handshake...";
        // Don't change state yet - wait for encrypted() signal
    } else {
        // Non-TLS: need to send SHA-384 password hash
        qDebug() << "Socket connected, sending authentication...";
        setState(Authenticating);
        sendAuthentication();
        m_authTimer->start(K4Protocol::AUTH_TIMEOUT_MS);
    }
}

void TcpClient::onSocketEncrypted() {
    // TLS handshake completed successfully
    QSslCipher negotiated = m_socket->sessionCipher();
    qDebug() << "=== TLS/PSK Connection Established ===";
    qDebug() << "  Negotiated cipher:" << negotiated.name();
    qDebug() << "  Protocol:" << negotiated.protocolString();
    qDebug() << "  Key exchange:" << negotiated.keyExchangeMethod();
    qDebug() << "  Encryption:" << negotiated.encryptionMethod();
    setState(Authenticating);
    // Start auth timeout - waiting for first packet to confirm connection works
    m_authTimer->start(K4Protocol::AUTH_TIMEOUT_MS);
    // Note: For TLS/PSK, no additional password auth needed - data flows immediately
}

void TcpClient::onSocketDisconnected() {
    qDebug() << "Socket disconnected";
    stopPingTimer();
    m_authTimer->stop();

    if (m_state == Authenticating && !m_authResponseReceived) {
        emit authenticationFailed();
        emit errorOccurred("Authentication failed - connection closed by radio");
    }

    setState(Disconnected);
}

void TcpClient::onReadyRead() {
    QByteArray data = m_socket->readAll();
    m_protocol->parse(data);
}

void TcpClient::onSocketError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    stopPingTimer();
    m_authTimer->stop();

    QString errorMsg = m_socket->errorString();
    qDebug() << "Socket error:" << errorMsg;

    if (m_state == Authenticating) {
        emit authenticationFailed();
    }

    emit errorOccurred(errorMsg);
    setState(Disconnected);
}

void TcpClient::onSslErrors(const QList<QSslError> &errors) {
    // Log SSL errors but continue - PSK doesn't use certificates so some errors are expected
    for (const QSslError &error : errors) {
        qDebug() << "SSL error (ignored for PSK):" << error.errorString();
    }
    // Ignore all SSL errors for PSK connections (no certificate verification)
    m_socket->ignoreSslErrors();
}

void TcpClient::onPreSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *authenticator) {
    qDebug() << "PSK authentication requested, identity hint:" << authenticator->identityHint();

    // Set the identity (can be empty for K4) and the pre-shared key
    authenticator->setIdentity(QByteArray()); // Empty identity
    authenticator->setPreSharedKey(m_psk.toUtf8());

    qDebug() << "PSK credentials provided";
}

void TcpClient::onAuthTimeout() {
    if (m_state == Authenticating && !m_authResponseReceived) {
        qDebug() << "Authentication timeout";
        emit authenticationFailed();
        emit errorOccurred("Authentication timeout - no response from radio");
        disconnectFromHost();
    }
}

void TcpClient::onPingTimer() {
    if (m_state == Connected) {
        sendCAT("PING;");
    }
}

void TcpClient::onCatResponse(const QString &response) {
    Q_UNUSED(response);
    // Verbose logging removed for performance
}

void TcpClient::sendAuthentication() {
    // Build SHA-384 hash of password as hex string
    QByteArray authData = Protocol::buildAuthData(m_password);
    qDebug() << "Sending auth hash (" << authData.size() << "bytes)";

    // Send raw auth data (not wrapped in K4 packet - just the hex string)
    m_socket->write(authData);
    m_socket->flush();
    // Radio will respond with packets, which triggers auth success and init sequence
}

void TcpClient::startPingTimer() {
    m_pingTimer->start();
}

void TcpClient::stopPingTimer() {
    m_pingTimer->stop();
}
