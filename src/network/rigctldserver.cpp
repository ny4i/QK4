#include "rigctldserver.h"
#include "models/radiostate.h"
#include <QDateTime>
#include <QDebug>

RigctldServer::RigctldServer(RadioState *state, QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this)), m_radioState(state) {
    connect(m_server, &QTcpServer::newConnection, this, &RigctldServer::onNewConnection);
}

RigctldServer::~RigctldServer() {
    stop();
}

bool RigctldServer::start(quint16 port) {
    if (m_server->isListening()) {
        if (m_server->serverPort() == port) {
            return true; // Already listening on this port
        }
        stop();
    }

    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        emit errorOccurred(QString("Failed to start rigctld server: %1").arg(m_server->errorString()));
        return false;
    }

    m_port = port;
    qDebug() << "RigctldServer: Listening on port" << port;
    emit started(port);
    return true;
}

void RigctldServer::stop() {
    // Close all client connections
    for (QTcpSocket *client : m_clients) {
        client->disconnectFromHost();
    }
    m_clients.clear();
    m_clientBuffers.clear();

    if (m_server->isListening()) {
        m_server->close();
        m_port = 0;
        qDebug() << "RigctldServer: Stopped";
        emit stopped();
    }
}

bool RigctldServer::isListening() const {
    return m_server->isListening();
}

quint16 RigctldServer::port() const {
    return m_port;
}

int RigctldServer::clientCount() const {
    return m_clients.count();
}

void RigctldServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        m_clients.append(client);
        m_clientBuffers[client] = QByteArray();

        connect(client, &QTcpSocket::readyRead, this, &RigctldServer::onClientData);
        connect(client, &QTcpSocket::disconnected, this, &RigctldServer::onClientDisconnected);

        QString address = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
        qDebug() << "RigctldServer: Client connected from" << address;
        emit clientConnected(address);
    }
}

void RigctldServer::onClientData() {
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client)
        return;

    // Append new data to client's buffer
    m_clientBuffers[client].append(client->readAll());

    // Process complete commands (newline-terminated)
    QByteArray &buffer = m_clientBuffers[client];
    while (buffer.contains('\n')) {
        int pos = buffer.indexOf('\n');
        QString command = QString::fromUtf8(buffer.left(pos)).trimmed();
        buffer.remove(0, pos + 1);

        if (!command.isEmpty()) {
            qDebug() << "RigctldServer: Received command:" << command;
            QString response = handleCommand(command);
            if (!response.isEmpty()) {
                qDebug() << "RigctldServer: Sending response:" << response.trimmed();
                client->write(response.toUtf8());
            }
        }
    }
}

void RigctldServer::onClientDisconnected() {
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client)
        return;

    QString address = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
    qDebug() << "RigctldServer: Client disconnected from" << address;

    m_clients.removeOne(client);
    m_clientBuffers.remove(client);
    client->deleteLater();

    emit clientDisconnected(address);
}

QString RigctldServer::handleCommand(const QString &cmd) {
    // Check for extended response mode (+ prefix)
    bool extended = cmd.startsWith('+');
    QString command = extended ? cmd.mid(1) : cmd;

    // Parse command and arguments
    QString cmdChar = command.left(1);
    QString args = command.mid(1).trimmed();

    // Handle backslash commands
    if (command.startsWith("\\")) {
        QString longCmd = command.mid(1).split(' ').first().toLower();
        args = command.mid(1 + longCmd.length()).trimmed();

        if (longCmd == "dump_state") {
            return handleDumpState();
        } else if (longCmd == "get_freq" || longCmd == "f") {
            return handleGetFrequency(extended);
        } else if (longCmd == "set_freq" || longCmd == "F") {
            return handleSetFrequency(args);
        } else if (longCmd == "get_mode" || longCmd == "m") {
            return handleGetMode(extended);
        } else if (longCmd == "set_mode" || longCmd == "M") {
            return handleSetMode(args);
        } else if (longCmd == "get_ptt" || longCmd == "t") {
            return handleGetPtt(extended);
        } else if (longCmd == "set_ptt" || longCmd == "T") {
            return handleSetPtt(args);
        } else if (longCmd == "get_vfo" || longCmd == "v") {
            return handleGetVfo(extended);
        } else if (longCmd == "set_vfo" || longCmd == "V") {
            // Accept VFO switch commands but we always report VFOA
            // K4Controller manages both VFOs simultaneously
            return "RPRT 0\n";
        } else if (longCmd == "get_split_vfo" || longCmd == "s") {
            return handleGetSplit(extended);
        } else if (longCmd == "set_split_vfo" || longCmd == "S") {
            return handleSetSplit(args);
        } else if (longCmd == "get_split_freq" || longCmd == "i") {
            return handleGetSplitFreq(extended);
        } else if (longCmd == "set_split_freq" || longCmd == "I") {
            return handleSetSplitFreq(args);
        } else if (longCmd == "get_split_mode" || longCmd == "x") {
            return handleGetSplitMode(extended);
        } else if (longCmd == "get_rit" || longCmd == "j") {
            return handleGetRit(extended);
        } else if (longCmd == "set_rit" || longCmd == "J") {
            return handleSetRit(args);
        } else if (longCmd == "get_xit" || longCmd == "z") {
            return handleGetXit(extended);
        } else if (longCmd == "set_xit" || longCmd == "Z") {
            return handleSetXit(args);
        } else if (longCmd == "get_ant" || longCmd == "y") {
            return handleGetAntenna(extended);
        } else if (longCmd == "set_ant" || longCmd == "Y") {
            return handleSetAntenna(args);
        } else if (longCmd == "get_ts" || longCmd == "n") {
            return handleGetTuningStep(extended);
        } else if (longCmd == "get_level" || longCmd == "l") {
            return handleGetLevel(args, extended);
        } else if (longCmd == "set_level" || longCmd == "L") {
            return handleSetLevel(args);
        } else if (longCmd == "get_func" || longCmd == "u") {
            return handleGetFunc(args, extended);
        } else if (longCmd == "set_func" || longCmd == "U") {
            return handleSetFunc(args);
        } else if (longCmd == "get_vfo_info") {
            return handleGetVfoInfo(args, extended);
        } else if (longCmd == "chk_vfo") {
            // VFO mode check - return 0 (disabled, we use VFOA always)
            return extended ? "chk_vfo:\nChkVFO: 0\nRPRT 0\n" : "0\n";
        } else if (longCmd == "get_rig_info") {
            return handleGetRigInfo();
        } else if (longCmd == "get_info") {
            return extended ? "get_info:\nInfo: Elecraft K4\nRPRT 0\n" : "Elecraft K4\n";
        } else if (longCmd == "get_trn") {
            // Transceive mode - 0 = off
            return extended ? "get_trn:\nTransceive: 0\nRPRT 0\n" : "0\n";
        } else if (longCmd == "set_trn") {
            // Accept but ignore transceive mode setting
            return extended ? "set_trn:\nRPRT 0\n" : "RPRT 0\n";
        } else if (longCmd == "get_powerstat") {
            // Power state - 1 = on (we're connected, so radio must be on)
            return extended ? "get_powerstat:\nPower Status: 1\nRPRT 0\n" : "1\n";
        } else if (longCmd == "set_powerstat") {
            // Accept but ignore - can't control power over network
            return extended ? "set_powerstat:\nRPRT 0\n" : "RPRT 0\n";
        } else if (longCmd == "get_lock_mode") {
            // Lock mode - 0 = unlocked
            return extended ? "get_lock_mode:\nLocked: 0\nRPRT 0\n" : "0\n";
        } else if (longCmd == "set_lock_mode") {
            // Accept but ignore lock mode
            return extended ? "set_lock_mode:\nRPRT 0\n" : "RPRT 0\n";
        } else if (longCmd.endsWith("?")) {
            // Query commands - return empty capability (RPRT 0)
            // Echo the command without the ? for extended mode
            QString cmdBase = longCmd.left(longCmd.length() - 1);
            return extended ? QString("%1:\nRPRT 0\n").arg(cmdBase) : "RPRT 0\n";
        } else if (args == "?") {
            // Capability query with ? as argument - return RPRT 0 for any command
            return extended ? QString("%1:\nRPRT 0\n").arg(longCmd) : "RPRT 0\n";
        } else if (longCmd == "get_level" || longCmd == "set_level" || longCmd == "get_func" || longCmd == "set_func" ||
                   longCmd == "vfo_op") {
            // Unsupported commands - return RPRT 0 (no error, just not supported)
            return extended ? QString("%1:\nRPRT 0\n").arg(longCmd) : "RPRT 0\n";
        }
        // Unknown long command
        return "RPRT -1\n";
    }

    // Single character commands
    if (cmdChar == "f") {
        return handleGetFrequency(extended);
    } else if (cmdChar == "F") {
        return handleSetFrequency(args);
    } else if (cmdChar == "m") {
        return handleGetMode(extended);
    } else if (cmdChar == "M") {
        return handleSetMode(args);
    } else if (cmdChar == "t") {
        return handleGetPtt(extended);
    } else if (cmdChar == "T") {
        return handleSetPtt(args);
    } else if (cmdChar == "v") {
        return handleGetVfo(extended);
    } else if (cmdChar == "V") {
        return "RPRT 0\n"; // Accept VFO switch
    } else if (cmdChar == "s") {
        return handleGetSplit(extended);
    } else if (cmdChar == "S") {
        return handleSetSplit(args);
    } else if (cmdChar == "i") {
        return handleGetSplitFreq(extended);
    } else if (cmdChar == "I") {
        return handleSetSplitFreq(args);
    } else if (cmdChar == "x") {
        return handleGetSplitMode(extended);
    } else if (cmdChar == "j") {
        return handleGetRit(extended);
    } else if (cmdChar == "J") {
        return handleSetRit(args);
    } else if (cmdChar == "z") {
        return handleGetXit(extended);
    } else if (cmdChar == "Z") {
        return handleSetXit(args);
    } else if (cmdChar == "y") {
        return handleGetAntenna(extended);
    } else if (cmdChar == "Y") {
        return handleSetAntenna(args);
    } else if (cmdChar == "n") {
        return handleGetTuningStep(extended);
    } else if (cmdChar == "l") {
        return handleGetLevel(args, extended);
    } else if (cmdChar == "L") {
        return handleSetLevel(args);
    } else if (cmdChar == "u") {
        return handleGetFunc(args, extended);
    } else if (cmdChar == "U") {
        return handleSetFunc(args);
    } else if (cmdChar == "q") {
        // Quit - client should disconnect
        return QString();
    }

    // Unknown command
    return "RPRT -1\n";
}

QString RigctldServer::handleGetFrequency(bool extended) const {
    quint64 freq = m_radioState->frequency();

    // Use pending frequency if recently set (avoids set/get mismatch delays)
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_pendingFrequency > 0 && (now - m_pendingFrequencyTime) < PENDING_TIMEOUT_MS) {
        freq = m_pendingFrequency;
    }

    if (extended) {
        return QString("get_freq:\nFrequency: %1\nRPRT 0\n").arg(freq);
    }
    return QString("%1\n").arg(freq);
}

QString RigctldServer::handleSetFrequency(const QString &args) {
    bool ok;
    // Handle both integer and floating point frequencies (WSJT-X sends decimals)
    double freqDouble = args.toDouble(&ok);
    if (!ok || freqDouble <= 0) {
        return "RPRT -1\n"; // Invalid parameter
    }
    quint64 freq = static_cast<quint64>(freqDouble);

    // Cache pending frequency to avoid set/get mismatch delays
    m_pendingFrequency = freq;
    m_pendingFrequencyTime = QDateTime::currentMSecsSinceEpoch();

    emit frequencyRequested(freq);
    return "RPRT 0\n";
}

QString RigctldServer::handleGetMode(bool extended) const {
    int modeInt = m_radioState->mode();

    // Use pending mode if recently set (avoids set/get mismatch delays)
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_pendingMode >= 0 && (now - m_pendingModeTime) < PENDING_TIMEOUT_MS) {
        modeInt = m_pendingMode;
    }

    QString mode = modeToHamlib(modeInt);
    int bandwidth = m_radioState->filterBandwidth();

    if (extended) {
        return QString("get_mode:\nMode: %1\nPassband: %2\nRPRT 0\n").arg(mode).arg(bandwidth);
    }
    return QString("%1\n%2\n").arg(mode).arg(bandwidth);
}

QString RigctldServer::handleSetMode(const QString &args) {
    QStringList parts = args.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return "RPRT -1\n";
    }

    QString mode = parts.at(0);
    int bandwidth = parts.size() > 1 ? parts.at(1).toInt() : 0;

    // Cache pending mode to avoid set/get mismatch delays
    m_pendingMode = hamlibToMode(mode);
    m_pendingModeTime = QDateTime::currentMSecsSinceEpoch();

    emit modeRequested(mode, bandwidth);
    return "RPRT 0\n";
}

QString RigctldServer::handleGetPtt(bool extended) const {
    int ptt = m_radioState->isTransmitting() ? 1 : 0;
    if (extended) {
        return QString("get_ptt:\nPTT: %1\nRPRT 0\n").arg(ptt);
    }
    return QString("%1\n").arg(ptt);
}

QString RigctldServer::handleSetPtt(const QString &args) {
    bool ok;
    int ptt = args.toInt(&ok);
    if (!ok) {
        return "RPRT -1\n";
    }

    emit pttRequested(ptt != 0);
    return "RPRT 0\n";
}

QString RigctldServer::handleGetVfo(bool extended) const {
    // K4Controller always operates on VFO A
    if (extended) {
        return "get_vfo:\nVFO: VFOA\nRPRT 0\n";
    }
    return "VFOA\n";
}

QString RigctldServer::handleGetSplit(bool extended) const {
    int split = m_radioState->splitEnabled() ? 1 : 0;
    if (extended) {
        return QString("get_split_vfo:\nSplit: %1\nTX VFO: VFOB\nRPRT 0\n").arg(split);
    }
    return QString("%1\nVFOB\n").arg(split);
}

QString RigctldServer::handleSetSplit(const QString &args) {
    // Format: "split tx_vfo" e.g., "1 VFOB" or just "1"
    QStringList parts = args.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return "RPRT -1\n";
    }

    bool ok;
    int split = parts.at(0).toInt(&ok);
    if (!ok) {
        return "RPRT -1\n";
    }

    // Emit signal to set split - MainWindow will send FT command
    emit splitRequested(split != 0);
    return "RPRT 0\n";
}

QString RigctldServer::handleGetSplitFreq(bool extended) const {
    // TX frequency is VFO B when split is enabled
    quint64 freq = m_radioState->vfoB();
    if (extended) {
        return QString("get_split_freq:\nTX Freq: %1\nRPRT 0\n").arg(freq);
    }
    return QString("%1\n").arg(freq);
}

QString RigctldServer::handleSetSplitFreq(const QString &args) {
    bool ok;
    quint64 freq = args.toULongLong(&ok);
    if (!ok || freq == 0) {
        return "RPRT -1\n";
    }

    // Set VFO B frequency (TX freq when split)
    emit splitFrequencyRequested(freq);
    return "RPRT 0\n";
}

QString RigctldServer::handleGetSplitMode(bool extended) const {
    // TX mode is VFO B mode
    QString mode = modeToHamlib(m_radioState->modeB());
    int width = m_radioState->filterBandwidthB();
    if (extended) {
        return QString("get_split_mode:\nTX Mode: %1\nTX Passband: %2\nRPRT 0\n").arg(mode).arg(width);
    }
    return QString("%1\n%2\n").arg(mode).arg(width);
}

QString RigctldServer::handleGetRit(bool extended) const {
    int offset = m_radioState->ritEnabled() ? m_radioState->ritXitOffset() : 0;
    if (extended) {
        return QString("get_rit:\nRIT: %1\nRPRT 0\n").arg(offset);
    }
    return QString("%1\n").arg(offset);
}

QString RigctldServer::handleSetRit(const QString &args) {
    bool ok;
    int offset = args.toInt(&ok);
    if (!ok) {
        return "RPRT -1\n";
    }

    emit ritRequested(offset);
    return "RPRT 0\n";
}

QString RigctldServer::handleGetXit(bool extended) const {
    int offset = m_radioState->xitEnabled() ? m_radioState->ritXitOffset() : 0;
    if (extended) {
        return QString("get_xit:\nXIT: %1\nRPRT 0\n").arg(offset);
    }
    return QString("%1\n").arg(offset);
}

QString RigctldServer::handleSetXit(const QString &args) {
    bool ok;
    int offset = args.toInt(&ok);
    if (!ok) {
        return "RPRT -1\n";
    }

    emit xitRequested(offset);
    return "RPRT 0\n";
}

QString RigctldServer::handleGetAntenna(bool extended) const {
    // Return TX antenna (1-indexed in Hamlib)
    int ant = m_radioState->txAntenna();
    if (extended) {
        return QString("get_ant:\nAntenna: %1\nRPRT 0\n").arg(ant);
    }
    return QString("%1\n").arg(ant);
}

QString RigctldServer::handleSetAntenna(const QString &args) {
    bool ok;
    int ant = args.toInt(&ok);
    if (!ok || ant < 1 || ant > 4) {
        return "RPRT -1\n";
    }

    emit antennaRequested(ant);
    return "RPRT 0\n";
}

QString RigctldServer::handleGetTuningStep(bool extended) const {
    // K4 tuning steps: 0=1Hz, 1=10Hz, 2=20Hz, 3=50Hz, 4=100Hz, 5=500Hz
    static const int steps[] = {1, 10, 20, 50, 100, 500};
    int stepIndex = m_radioState->tuningStep();
    int stepHz = (stepIndex >= 0 && stepIndex <= 5) ? steps[stepIndex] : 10;

    if (extended) {
        return QString("get_ts:\nTuning Step: %1\nRPRT 0\n").arg(stepHz);
    }
    return QString("%1\n").arg(stepHz);
}

QString RigctldServer::handleGetLevel(const QString &level, bool extended) const {
    QString lvl = level.toUpper();
    QString value;

    if (lvl == "RFPOWER" || lvl == "RF") {
        // RF power as ratio 0.0-1.0 (Hamlib convention)
        double watts = m_radioState->rfPower();
        double ratio = watts / 100.0; // K4 max 100W
        value = QString::number(ratio, 'f', 2);
    } else if (lvl == "AF") {
        // AF gain - we don't track this, return 0.5
        value = "0.50";
    } else if (lvl == "RF") {
        // RF gain 0-100 as ratio
        double ratio = m_radioState->rfGain() / 100.0;
        value = QString::number(ratio, 'f', 2);
    } else if (lvl == "SQL") {
        // Squelch 0-100 as ratio
        double ratio = m_radioState->squelchLevel() / 100.0;
        value = QString::number(ratio, 'f', 2);
    } else if (lvl == "MICGAIN") {
        // Mic gain 0-80 as ratio
        double ratio = m_radioState->micGain() / 80.0;
        value = QString::number(ratio, 'f', 2);
    } else if (lvl == "COMP") {
        // Compression 0-30 dB
        value = QString::number(m_radioState->compression());
    } else if (lvl == "KEYSPD") {
        // Keyer speed in WPM
        value = QString::number(m_radioState->keyerSpeed());
    } else if (lvl == "CWPITCH") {
        // CW pitch in Hz
        value = QString::number(m_radioState->cwPitch());
    } else if (lvl == "IF") {
        // IF shift in Hz
        value = QString::number(m_radioState->ifShift());
    } else if (lvl == "STRENGTH") {
        // S-meter in dB (0 = S9)
        double db = m_radioState->sMeter() - 54; // Convert to dB relative to S9
        value = QString::number(static_cast<int>(db));
    } else if (lvl == "SWR") {
        // SWR ratio
        value = QString::number(m_radioState->swrMeter(), 'f', 1);
    } else if (lvl == "ALC") {
        // ALC level
        value = QString::number(m_radioState->alcMeter());
    } else if (lvl == "RFPOWER_METER" || lvl == "RFPOWER_METER_WATTS") {
        // Forward power in watts
        value = QString::number(m_radioState->forwardPower(), 'f', 1);
    } else if (lvl == "PREAMP") {
        // Preamp level
        value = QString::number(m_radioState->preamp());
    } else if (lvl == "ATT") {
        // Attenuator level in dB
        value = QString::number(m_radioState->attenuatorLevel());
    } else if (lvl == "AGC") {
        // AGC speed: 0=off, 1=slow, 2=fast (Hamlib: 0=off, 1=superfast, 2=fast, 3=slow, 4=user, 5=medium, 6=auto)
        int agc = m_radioState->agcSpeed();
        // Map K4 AGC to Hamlib: 0->0, 1->3, 2->2
        int hamlibAgc = (agc == 0) ? 0 : (agc == 1) ? 3 : 2;
        value = QString::number(hamlibAgc);
    } else if (lvl == "NR") {
        // Noise reduction level as ratio
        double ratio = m_radioState->noiseReductionLevel() / 10.0; // K4: 0-10
        value = QString::number(ratio, 'f', 2);
    } else if (lvl == "NB") {
        // Noise blanker level as ratio
        double ratio = m_radioState->noiseBlankerLevel() / 10.0; // K4: 0-10
        value = QString::number(ratio, 'f', 2);
    } else {
        // Unknown level
        return "RPRT -1\n";
    }

    if (extended) {
        return QString("get_level:\n%1: %2\nRPRT 0\n").arg(lvl).arg(value);
    }
    return QString("%1\n").arg(value);
}

QString RigctldServer::handleSetLevel(const QString &args) {
    QStringList parts = args.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        return "RPRT -1\n";
    }

    QString lvl = parts.at(0).toUpper();
    QString valStr = parts.at(1);

    if (lvl == "RFPOWER") {
        bool ok;
        double ratio = valStr.toDouble(&ok);
        if (!ok)
            return "RPRT -1\n";
        int watts = static_cast<int>(ratio * 100.0);
        emit levelRequested(lvl, watts);
    } else if (lvl == "KEYSPD") {
        bool ok;
        int wpm = valStr.toInt(&ok);
        if (!ok)
            return "RPRT -1\n";
        emit levelRequested(lvl, wpm);
    } else if (lvl == "CWPITCH") {
        bool ok;
        int pitch = valStr.toInt(&ok);
        if (!ok)
            return "RPRT -1\n";
        emit levelRequested(lvl, pitch);
    } else if (lvl == "MICGAIN") {
        bool ok;
        double ratio = valStr.toDouble(&ok);
        if (!ok)
            return "RPRT -1\n";
        int gain = static_cast<int>(ratio * 80.0);
        emit levelRequested(lvl, gain);
    } else if (lvl == "COMP") {
        bool ok;
        int comp = valStr.toInt(&ok);
        if (!ok)
            return "RPRT -1\n";
        emit levelRequested(lvl, comp);
    } else if (lvl == "IF") {
        bool ok;
        int shift = valStr.toInt(&ok);
        if (!ok)
            return "RPRT -1\n";
        emit levelRequested(lvl, shift);
    } else if (lvl == "NR") {
        bool ok;
        double ratio = valStr.toDouble(&ok);
        if (!ok)
            return "RPRT -1\n";
        int level = static_cast<int>(ratio * 10.0);
        emit levelRequested(lvl, level);
    } else if (lvl == "NB") {
        bool ok;
        double ratio = valStr.toDouble(&ok);
        if (!ok)
            return "RPRT -1\n";
        int level = static_cast<int>(ratio * 10.0);
        emit levelRequested(lvl, level);
    } else {
        // Unsupported level for SET
        return "RPRT -1\n";
    }

    return "RPRT 0\n";
}

QString RigctldServer::handleGetFunc(const QString &func, bool extended) const {
    QString fn = func.toUpper();
    int value = 0;

    if (fn == "VOX") {
        value = m_radioState->voxEnabled() ? 1 : 0;
    } else if (fn == "NB") {
        value = m_radioState->noiseBlankerEnabled() ? 1 : 0;
    } else if (fn == "NR") {
        value = m_radioState->noiseReductionEnabled() ? 1 : 0;
    } else if (fn == "ANF") {
        value = m_radioState->autoNotchEnabled() ? 1 : 0;
    } else if (fn == "MN") {
        value = m_radioState->manualNotchEnabled() ? 1 : 0;
    } else if (fn == "FBKIN") {
        value = m_radioState->qskEnabled() ? 1 : 0;
    } else if (fn == "TUNER") {
        value = (m_radioState->atuMode() == 2) ? 1 : 0; // 2 = auto
    } else {
        // Unknown function
        return "RPRT -1\n";
    }

    if (extended) {
        return QString("get_func:\n%1: %2\nRPRT 0\n").arg(fn).arg(value);
    }
    return QString("%1\n").arg(value);
}

QString RigctldServer::handleSetFunc(const QString &args) {
    QStringList parts = args.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        return "RPRT -1\n";
    }

    QString fn = parts.at(0).toUpper();
    bool ok;
    int value = parts.at(1).toInt(&ok);
    if (!ok) {
        return "RPRT -1\n";
    }

    emit funcRequested(fn, value != 0);
    return "RPRT 0\n";
}

QString RigctldServer::handleGetVfoInfo(const QString &vfo, bool extended) const {
    // Get values based on which VFO is requested
    bool isVfoB = vfo.toUpper().contains("B");
    QString vfoName = isVfoB ? "VFOB" : "VFOA";

    quint64 freq = isVfoB ? m_radioState->vfoB() : m_radioState->frequency();
    QString mode = modeToHamlib(isVfoB ? m_radioState->modeB() : m_radioState->mode());
    int width = isVfoB ? m_radioState->filterBandwidthB() : m_radioState->filterBandwidth();
    int split = m_radioState->splitEnabled() ? 1 : 0;

    if (extended) {
        // Extended mode: labeled values only (Hamlib rigctld format)
        QStringList lines;
        lines << QString("Freq: %1").arg(freq);
        lines << QString("Mode: %1").arg(mode);
        lines << QString("Width: %1").arg(width);
        lines << QString("Split: %1").arg(split);
        lines << "SatMode: 0";
        lines << "RPRT 0";
        return lines.join("\n") + "\n";
    }

    // Non-extended mode: just values
    QStringList lines;
    lines << QString("%1").arg(freq);
    lines << mode;
    lines << QString("%1").arg(width);
    lines << QString("%1").arg(split);
    lines << "0"; // SatMode
    return lines.join("\n") + "\n";
}

QString RigctldServer::handleGetRigInfo() const {
    // Return rig info in the expected format
    quint64 freqA = m_radioState->frequency();
    quint64 freqB = m_radioState->vfoB();
    QString modeA = modeToHamlib(m_radioState->mode());
    QString modeB = modeToHamlib(m_radioState->modeB());
    int widthA = m_radioState->filterBandwidth();
    int widthB = m_radioState->filterBandwidthB();
    int split = m_radioState->splitEnabled() ? 1 : 0;

    // Format follows rigctld protocol with command echo
    QString info = QString("get_rig_info:\n"
                           "VFO=VFOA Freq=%1 Mode=%2 Width=%3\n"
                           "VFO=VFOB Freq=%4 Mode=%5 Width=%6\n"
                           "Split=%7 SatMode=0\n"
                           "Rig=Elecraft K4\n"
                           "RPRT 0\n")
                       .arg(freqA)
                       .arg(modeA)
                       .arg(widthA)
                       .arg(freqB)
                       .arg(modeB)
                       .arg(widthB)
                       .arg(split);
    return info;
}

QString RigctldServer::handleDumpState() const {
    // dump_state response format for Hamlib netrigctl protocol
    // Format: startf endf modes low_power high_power vfo ant (7 fields)
    QStringList lines;
    lines << "1"; // Protocol version
    lines << "2"; // Rig model (2 = NET rigctl)
    lines << "2"; // ITU region
    // RX range list: startf endf modes low_power high_power vfo ant
    lines << "150000 60000000 0x1ff -1 -1 0x40000003 0x00"; // RX: 150kHz-60MHz, all modes
    lines << "0 0 0 0 0 0 0";                               // RX range end marker
    // TX range list
    lines << "1800000 2000000 0x1ff 5 110 0x40000003 0x00";   // TX: 160m
    lines << "3500000 4000000 0x1ff 5 110 0x40000003 0x00";   // TX: 80m
    lines << "5330500 5406400 0x1ff 5 110 0x40000003 0x00";   // TX: 60m
    lines << "7000000 7300000 0x1ff 5 110 0x40000003 0x00";   // TX: 40m
    lines << "10100000 10150000 0x1ff 5 110 0x40000003 0x00"; // TX: 30m
    lines << "14000000 14350000 0x1ff 5 110 0x40000003 0x00"; // TX: 20m
    lines << "18068000 18168000 0x1ff 5 110 0x40000003 0x00"; // TX: 17m
    lines << "21000000 21450000 0x1ff 5 110 0x40000003 0x00"; // TX: 15m
    lines << "24890000 24990000 0x1ff 5 110 0x40000003 0x00"; // TX: 12m
    lines << "28000000 29700000 0x1ff 5 110 0x40000003 0x00"; // TX: 10m
    lines << "50000000 54000000 0x1ff 5 110 0x40000003 0x00"; // TX: 6m
    lines << "0 0 0 0 0 0 0";                                 // TX range end marker
    // Tuning steps: modes ts
    lines << "0x1ff 1";    // 1 Hz step for all modes
    lines << "0x1ff 10";   // 10 Hz step
    lines << "0x1ff 100";  // 100 Hz step
    lines << "0x1ff 1000"; // 1 kHz step
    lines << "0 0";        // Tuning steps end
    // Filters: modes width
    lines << "0x22 500";           // CW 500Hz
    lines << "0x22 400";           // CW 400Hz
    lines << "0x03 2400";          // SSB 2400Hz
    lines << "0x03 2700";          // SSB 2700Hz
    lines << "0x0c 6000";          // AM/FM 6kHz
    lines << "0 0";                // Filters end
    lines << "9999";               // Max RIT (Hz)
    lines << "9999";               // Max XIT (Hz)
    lines << "1000";               // Max IF shift (Hz)
    lines << "0";                  // Announces
    lines << "0 10 0 0 0 0 0";     // Preamp levels (7 values)
    lines << "0 6 12 18 0 0 0";    // Attenuator levels (7 values)
    lines << "0xffffffffffffffff"; // Has get func
    lines << "0xffffffffffffffff"; // Has set func
    lines << "0xffffffffffffffff"; // Has get level
    lines << "0xffffffffffffffff"; // Has set level
    lines << "0x0";                // Has get parm
    lines << "0x0";                // Has set parm
    // Protocol v1 key=value pairs
    lines << "vfo_ops=0x0";
    lines << "ptt_type=0x1"; // PTT via CAT command (RIG_PTT_RIG)
    lines << "targetable_vfo=0x0";
    lines << "has_set_vfo=0";
    lines << "has_get_vfo=0";
    lines << "has_set_freq=1";
    lines << "has_get_freq=1";
    lines << "timeout=1000";
    lines << "rig_model=2";
    lines << "done"; // Terminates protocol v1 handshake

    return lines.join("\n") + "\n";
}

QString RigctldServer::modeToHamlib(int mode) const {
    switch (mode) {
    case RadioState::LSB:
        return "LSB";
    case RadioState::USB:
        return "USB";
    case RadioState::CW:
        return "CW";
    case RadioState::CW_R:
        return "CWR";
    case RadioState::AM:
        return "AM";
    case RadioState::FM:
        return "FM";
    case RadioState::DATA:
        return "USBD"; // USB Data - some loggers prefer this over PKTUSB
    case RadioState::DATA_R:
        return "LSBD"; // LSB Data - some loggers prefer this over PKTLSB
    default:
        return "USB";
    }
}

int RigctldServer::hamlibToMode(const QString &mode) const {
    QString m = mode.toUpper();
    if (m == "LSB")
        return RadioState::LSB;
    if (m == "USB")
        return RadioState::USB;
    if (m == "CW")
        return RadioState::CW;
    if (m == "CWR" || m == "CW-R")
        return RadioState::CW_R;
    if (m == "AM")
        return RadioState::AM;
    if (m == "FM")
        return RadioState::FM;
    if (m == "PKTUSB" || m == "PKT-U" || m == "DIGU")
        return RadioState::DATA;
    if (m == "PKTLSB" || m == "PKT-L" || m == "DIGL")
        return RadioState::DATA_R;
    // Default to USB
    return RadioState::USB;
}
