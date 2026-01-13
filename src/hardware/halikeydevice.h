#ifndef HALIKEYDEVICE_H
#define HALIKEYDEVICE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>

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

private slots:
    void pollSignals();

private:
    QSerialPort *m_serialPort = nullptr;
    QTimer *m_pollTimer = nullptr;

    // Polling interval - fast enough for responsive keying
    static const int POLL_INTERVAL_MS = 1;

    // Debounce - require stable state for N consecutive reads
    static const int DEBOUNCE_COUNT = 3; // 3ms at 1ms polling

    // Last known paddle states (debounced, what we report)
    bool m_lastDitState = false;
    bool m_lastDahState = false;

    // Raw states and debounce counters
    bool m_rawDitState = false;
    bool m_rawDahState = false;
    int m_ditDebounceCounter = 0;
    int m_dahDebounceCounter = 0;
};

#endif // HALIKEYDEVICE_H
