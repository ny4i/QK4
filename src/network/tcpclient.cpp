#include "tcpclient.h"
#include <QDebug>

TcpClient::TcpClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)), m_protocol(new Protocol(this)), m_authTimer(new QTimer(this)),
      m_pingTimer(new QTimer(this)), m_port(K4Protocol::DEFAULT_PORT), m_state(Disconnected),
      m_authResponseReceived(false) {
    // Socket signals
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpClient::onSocketError);

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
            sendCAT("RDY;"); // Ready - triggers comprehensive state dump
            sendCAT("K41;"); // Enable advanced K4 protocol mode
            sendCAT("AI4;"); // Enable Auto Information mode 4 - CRITICAL for streaming updates
            sendCAT("ER1;"); // Request long format error messages
            sendCAT("EM3;"); // Set audio to Opus Float 32-bit mode (required for audio streaming)
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

void TcpClient::connectToHost(const QString &host, quint16 port, const QString &password) {
    if (m_state != Disconnected) {
        disconnectFromHost();
    }

    m_host = host;
    m_port = port;
    m_password = password;
    m_authResponseReceived = false;

    setState(Connecting);
    m_socket->connectToHost(host, port);
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
    qDebug() << "Socket connected, sending authentication...";
    setState(Authenticating);
    sendAuthentication();

    // Start auth timeout
    m_authTimer->start(K4Protocol::AUTH_TIMEOUT_MS);
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
