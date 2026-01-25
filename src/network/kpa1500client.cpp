#include "kpa1500client.h"
#include <QDebug>

// Poll commands from user's Node-RED flow
const QString KPA1500Client::POLL_COMMANDS =
    "^BN;^WS;^TM;^FS;^VI;^FC;^ON;^FL;^AN;^IP;^SN;^PC;^VM1;^VM2;^VM3;^VM5;^LR;^CR;^PWF;^PWR;^PWD;";

KPA1500Client::KPA1500Client(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)), m_pollTimer(new QTimer(this)), m_port(1500),
      m_state(Disconnected) {
    connect(m_socket, &QTcpSocket::connected, this, &KPA1500Client::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &KPA1500Client::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &KPA1500Client::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &KPA1500Client::onSocketError);
    connect(m_pollTimer, &QTimer::timeout, this, &KPA1500Client::onPollTimer);
}

KPA1500Client::~KPA1500Client() {
    stopPolling();
    // Abort socket directly without emitting signals during destruction
    // Don't call disconnectFromHost() which emits stateChanged/disconnected
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort();
    }
}

void KPA1500Client::connectToHost(const QString &host, quint16 port) {
    if (m_state != Disconnected) {
        disconnectFromHost();
    }

    m_host = host;
    m_port = port;
    m_receiveBuffer.clear();

    setState(Connecting);
    m_socket->connectToHost(host, port);
}

void KPA1500Client::disconnectFromHost() {
    stopPolling();
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->abort();
        }
    }
    setState(Disconnected);
}

bool KPA1500Client::isConnected() const {
    return m_state == Connected;
}

KPA1500Client::ConnectionState KPA1500Client::connectionState() const {
    return m_state;
}

void KPA1500Client::sendCommand(const QString &command) {
    if (m_state != Connected) {
        qWarning() << "KPA1500Client: Cannot send command, not connected";
        return;
    }

    QByteArray data = command.toLatin1();
    m_socket->write(data);
    m_socket->flush();
}

void KPA1500Client::startPolling(int intervalMs) {
    if (m_state == Connected && intervalMs > 0) {
        m_pollTimer->start(intervalMs);
        // Send initial poll immediately
        onPollTimer();
    }
}

void KPA1500Client::stopPolling() {
    m_pollTimer->stop();
}

void KPA1500Client::setState(ConnectionState state) {
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

void KPA1500Client::onSocketConnected() {
    qDebug() << "KPA1500Client: Connected to" << m_host << ":" << m_port;
    setState(Connected);
    emit connected();
}

void KPA1500Client::onSocketDisconnected() {
    qDebug() << "KPA1500Client: Disconnected";
    stopPolling();
    setState(Disconnected);
    emit disconnected();
}

void KPA1500Client::onReadyRead() {
    QByteArray data = m_socket->readAll();
    m_receiveBuffer.append(QString::fromLatin1(data));

    // Parse complete responses (terminated by ';')
    parseResponse(m_receiveBuffer);
}

void KPA1500Client::onSocketError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    QString errorString = m_socket->errorString();
    qWarning() << "KPA1500Client: Socket error:" << errorString;
    emit errorOccurred(errorString);

    stopPolling();
    setState(Disconnected);
}

void KPA1500Client::onPollTimer() {
    if (m_state == Connected) {
        sendCommand(POLL_COMMANDS);
    }
}

void KPA1500Client::parseResponse(const QString &response) {
    // Split by ';' and process each complete response
    int pos = 0;
    int endPos;

    while ((endPos = response.indexOf(';', pos)) != -1) {
        QString singleResponse = response.mid(pos, endPos - pos + 1);
        parseSingleResponse(singleResponse);
        pos = endPos + 1;
    }

    // Keep any incomplete data in the buffer
    m_receiveBuffer = response.mid(pos);
}

void KPA1500Client::parseSingleResponse(const QString &response) {
    // KPA1500 responses start with '^' and end with ';'
    if (!response.startsWith('^') || !response.endsWith(';')) {
        return;
    }

    // Remove ^ prefix and ; suffix
    QString cmd = response.mid(1, response.length() - 2);

    if (cmd.isEmpty()) {
        return;
    }

    // Parse based on command prefix
    // ^BN - Band Name
    if (cmd.startsWith("BN")) {
        QString band = cmd.mid(2);
        if (m_bandName != band) {
            m_bandName = band;
            emit bandChanged(band);
        }
    }
    // ^SN - Serial Number
    else if (cmd.startsWith("SN")) {
        m_serialNumber = cmd.mid(2);
    }
    // ^VI - Firmware Version Info
    else if (cmd.startsWith("VI")) {
        m_firmwareVersion = cmd.mid(2);
    }
    // ^ON - Operating State (0=Standby, 1=Operate)
    else if (cmd.startsWith("ON")) {
        int state = cmd.mid(2).toInt();
        OperatingState newState = (state == 1) ? StateOperate : StateStandby;
        if (m_operatingState != newState) {
            m_operatingState = newState;
            emit operatingStateChanged(newState);
        }
    }
    // ^FS - Fault Status (0=None, 1=Active, 2=History)
    else if (cmd.startsWith("FS")) {
        int status = cmd.mid(2).toInt();
        FaultStatus newStatus = static_cast<FaultStatus>(status);
        if (m_faultStatus != newStatus) {
            m_faultStatus = newStatus;
            emit faultStatusChanged(newStatus, m_faultCode);
        }
    }
    // ^FC - Fault Code
    else if (cmd.startsWith("FC")) {
        m_faultCode = cmd.mid(2);
        emit faultStatusChanged(m_faultStatus, m_faultCode);
    }
    // ^FL - Fault List (detailed fault info)
    else if (cmd.startsWith("FL")) {
        // Parse fault list if needed
    }
    // ^TM - Temperature (PA temperature in Celsius)
    else if (cmd.startsWith("TM")) {
        bool ok;
        double temp = cmd.mid(2).toDouble(&ok);
        if (ok && m_paTemperature != temp) {
            m_paTemperature = temp;
            emit paTemperatureChanged(temp);
        }
    }
    // ^WS - SWR
    else if (cmd.startsWith("WS")) {
        bool ok;
        double swr = cmd.mid(2).toDouble(&ok);
        if (ok) {
            // KPA1500 reports SWR as integer ratio * 10 (e.g., 15 = 1.5:1)
            swr = swr / 10.0;
            if (m_swr != swr) {
                m_swr = swr;
                emit swrChanged(swr);
            }
        }
    }
    // ^PWF - Forward Power (watts)
    else if (cmd.startsWith("PWF")) {
        bool ok;
        double power = cmd.mid(3).toDouble(&ok);
        if (ok && m_forwardPower != power) {
            m_forwardPower = power;
            emit powerChanged(m_forwardPower, m_reflectedPower, m_drivePower);
        }
    }
    // ^PWR - Reflected Power (watts)
    else if (cmd.startsWith("PWR")) {
        bool ok;
        double power = cmd.mid(3).toDouble(&ok);
        if (ok && m_reflectedPower != power) {
            m_reflectedPower = power;
            emit powerChanged(m_forwardPower, m_reflectedPower, m_drivePower);
        }
    }
    // ^PWD - Drive Power (watts)
    else if (cmd.startsWith("PWD")) {
        bool ok;
        double power = cmd.mid(3).toDouble(&ok);
        if (ok && m_drivePower != power) {
            m_drivePower = power;
            emit powerChanged(m_forwardPower, m_reflectedPower, m_drivePower);
        }
    }
    // ^VM1 - PA Voltage
    else if (cmd.startsWith("VM1")) {
        bool ok;
        double voltage = cmd.mid(3).toDouble(&ok);
        if (ok) {
            // VM1 reports in millivolts, convert to volts
            voltage = voltage / 1000.0;
            if (m_paVoltage != voltage) {
                m_paVoltage = voltage;
                emit paVoltageChanged(voltage);
            }
        }
    }
    // ^VM2 - PA Current
    else if (cmd.startsWith("VM2")) {
        bool ok;
        double current = cmd.mid(3).toDouble(&ok);
        if (ok) {
            // VM2 reports in milliamps, convert to amps
            current = current / 1000.0;
            if (m_paCurrent != current) {
                m_paCurrent = current;
                emit paCurrentChanged(current);
            }
        }
    }
    // ^AN - ATU Status
    else if (cmd.startsWith("AN")) {
        // Format: ANxy where x=present, y=active
        if (cmd.length() >= 4) {
            bool present = (cmd[2] == '1');
            bool active = (cmd[3] == '1');
            if (m_atuPresent != present || m_atuActive != active) {
                m_atuPresent = present;
                m_atuActive = active;
                emit atuStatusChanged(present, active);
            }
        }
    }
    // ^IP - Input Power (similar to drive power)
    else if (cmd.startsWith("IP")) {
        // Input power in watts
    }
    // ^PC - Power Control setting
    else if (cmd.startsWith("PC")) {
        // Power control percentage
    }
    // ^LR - Last Response code
    else if (cmd.startsWith("LR")) {
        // Response code from last command
    }
    // ^CR - Command Response
    else if (cmd.startsWith("CR")) {
        // Command response status
    }
    // ^VM3, ^VM5 - Additional voltage/current readings
    else if (cmd.startsWith("VM3") || cmd.startsWith("VM5")) {
        // Additional measurements (bias voltage, etc.)
    }
}
