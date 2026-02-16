#include "halikeydevice.h"
#include "halikeyworker.h"
#include <QDebug>

HalikeyDevice::HalikeyDevice(QObject *parent) : QObject(parent) {}

HalikeyDevice::~HalikeyDevice() {
    closePort();
}

bool HalikeyDevice::openPort(const QString &portName) {
    if (m_connected) {
        closePort();
    }

    m_portName = portName;
    m_lastDitState = false;
    m_lastDahState = false;

    // Create worker and thread
    m_worker = new HaliKeyWorker(portName);
    m_workerThread = new QThread(this);
    m_worker->moveToThread(m_workerThread);

    // Wire worker signals to device signals (queued cross-thread connections)
    connect(m_worker, &HaliKeyWorker::ditStateChanged, this, [this](bool pressed) {
        m_lastDitState = pressed;
        emit ditStateChanged(pressed);
    });
    connect(m_worker, &HaliKeyWorker::dahStateChanged, this, [this](bool pressed) {
        m_lastDahState = pressed;
        emit dahStateChanged(pressed);
    });
    connect(m_worker, &HaliKeyWorker::portOpened, this, [this]() {
        m_connected = true;
        emit connected();
    });
    connect(m_worker, &HaliKeyWorker::errorOccurred, this, [this](const QString &error) {
        qWarning() << "HalikeyDevice: Worker error -" << error;
        closePort();
        emit connectionError(error);
    });

    // Start worker when thread starts
    connect(m_workerThread, &QThread::started, m_worker, &HaliKeyWorker::start);

    // Clean up worker when thread finishes
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread->start();
    return true;
}

void HalikeyDevice::closePort() {
    if (m_worker) {
        m_worker->stop();
    }

    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(2000);
        m_workerThread->deleteLater();
        m_workerThread = nullptr;
    }

    m_worker = nullptr; // Deleted by QThread::finished → deleteLater

    bool wasConnected = m_connected;
    m_connected = false;
    m_lastDitState = false;
    m_lastDahState = false;

    if (wasConnected) {
        emit disconnected();
    }
}

bool HalikeyDevice::isConnected() const {
    return m_connected;
}

QString HalikeyDevice::portName() const {
    return m_portName;
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
