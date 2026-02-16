#ifndef HALIKEYDEVICE_H
#define HALIKEYDEVICE_H

#include <QObject>
#include <QSerialPortInfo>
#include <QString>
#include <QThread>

class HaliKeyWorker;

class HalikeyDevice : public QObject {
    Q_OBJECT

public:
    explicit HalikeyDevice(QObject *parent = nullptr);
    ~HalikeyDevice();

    // Port management
    bool openPort(const QString &portName);
    void closePort();
    bool isConnected() const;
    QString portName() const;

    // Available ports
    static QStringList availablePorts();

    // Current paddle state
    bool ditPressed() const;
    bool dahPressed() const;

signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);

    // Paddle state changes
    void ditStateChanged(bool pressed);
    void dahStateChanged(bool pressed);

private:
    QThread *m_workerThread = nullptr;
    HaliKeyWorker *m_worker = nullptr;

    QString m_portName;
    bool m_connected = false;
    bool m_lastDitState = false;
    bool m_lastDahState = false;
};

#endif // HALIKEYDEVICE_H
