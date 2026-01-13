#include "halikeydevice.h"
#include <QDebug>

HalikeyDevice::HalikeyDevice(QObject *parent)
    : QObject(parent), m_serialPort(new QSerialPort(this)), m_pollTimer(new QTimer(this)) {
    m_pollTimer->setInterval(POLL_INTERVAL_MS);
    connect(m_pollTimer, &QTimer::timeout, this, &HalikeyDevice::pollSignals);
}

HalikeyDevice::~HalikeyDevice() {
    closePort();
}

bool HalikeyDevice::openPort(const QString &portName) {
    if (m_serialPort->isOpen()) {
        closePort();
    }

    m_serialPort->setPortName(portName);

    // HaliKey doesn't need specific baud rate since we only read flow control signals,
    // but we need to open the port. Use a reasonable default.
    m_serialPort->setBaudRate(QSerialPort::Baud9600);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serialPort->open(QIODevice::ReadOnly)) {
        QString error = m_serialPort->errorString();
        qWarning() << "HalikeyDevice: Failed to open port" << portName << "-" << error;
        emit connectionError(error);
        return false;
    }

    // Reset paddle states and debounce counters
    m_lastDitState = false;
    m_lastDahState = false;
    m_rawDitState = false;
    m_rawDahState = false;
    m_ditDebounceCounter = 0;
    m_dahDebounceCounter = 0;

    // Start polling flow control signals
    m_pollTimer->start();

    qDebug() << "HalikeyDevice: Connected to" << portName;
    emit connected();
    return true;
}

void HalikeyDevice::closePort() {
    m_pollTimer->stop();

    if (m_serialPort->isOpen()) {
        QString portName = m_serialPort->portName();
        m_serialPort->close();
        qDebug() << "HalikeyDevice: Disconnected from" << portName;
        emit disconnected();
    }

    m_lastDitState = false;
    m_lastDahState = false;
}

bool HalikeyDevice::isConnected() const {
    return m_serialPort->isOpen();
}

QString HalikeyDevice::portName() const {
    return m_serialPort->portName();
}

QStringList HalikeyDevice::availablePorts() {
    QStringList ports;
    const auto portInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : portInfos) {
        ports.append(info.portName());
    }
    return ports;
}

bool HalikeyDevice::ditPressed() const {
    return m_lastDitState;
}

bool HalikeyDevice::dahPressed() const {
    return m_lastDahState;
}

void HalikeyDevice::pollSignals() {
    if (!m_serialPort->isOpen()) {
        return;
    }

    // Read flow control signals
    // HaliKey v1: CTS = dit (tip), DSR/DCD = dah (ring)
    QSerialPort::PinoutSignals pinState = m_serialPort->pinoutSignals();

    // Check for read error
    if (pinState == QSerialPort::NoSignal && m_serialPort->error() != QSerialPort::NoError) {
        QString error = m_serialPort->errorString();
        qWarning() << "HalikeyDevice: Error reading signals -" << error;
        closePort();
        emit connectionError(error);
        return;
    }

    // CTS = dit paddle (tip contact)
    bool ditState = (pinState & QSerialPort::ClearToSendSignal) != 0;

    // DSR = dah paddle (ring contact)
    bool dahState = (pinState & QSerialPort::DataSetReadySignal) != 0;

    // Debounce dit - require stable state for DEBOUNCE_COUNT consecutive reads
    if (ditState == m_rawDitState) {
        // Same as last raw read, increment counter
        if (m_ditDebounceCounter < DEBOUNCE_COUNT) {
            m_ditDebounceCounter++;
        }
        // If stable long enough and different from debounced state, update
        if (m_ditDebounceCounter >= DEBOUNCE_COUNT && ditState != m_lastDitState) {
            m_lastDitState = ditState;
            emit ditStateChanged(ditState);
        }
    } else {
        // State changed, reset counter
        m_rawDitState = ditState;
        m_ditDebounceCounter = 1;
    }

    // Debounce dah - same logic
    if (dahState == m_rawDahState) {
        if (m_dahDebounceCounter < DEBOUNCE_COUNT) {
            m_dahDebounceCounter++;
        }
        if (m_dahDebounceCounter >= DEBOUNCE_COUNT && dahState != m_lastDahState) {
            m_lastDahState = dahState;
            emit dahStateChanged(dahState);
        }
    } else {
        m_rawDahState = dahState;
        m_dahDebounceCounter = 1;
    }
}
