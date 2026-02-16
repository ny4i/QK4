#ifndef HALIKEYWORKER_H
#define HALIKEYWORKER_H

#include <QObject>
#include <QString>
#include <atomic>

class HaliKeyWorker : public QObject {
    Q_OBJECT

public:
    explicit HaliKeyWorker(const QString &portName, QObject *parent = nullptr);
    ~HaliKeyWorker();

public slots:
    void start(); // Called when thread starts — opens port, enters monitor loop
    void stop();  // Sets atomic flag to exit loop

signals:
    void ditStateChanged(bool pressed);
    void dahStateChanged(bool pressed);
    void errorOccurred(const QString &error);
    void portOpened();

private:
    void monitorLoop(); // Platform-specific main loop
    bool openNativePort();
    void closeNativePort();
    bool readPinState(bool &ditState, bool &dahState);

    QString m_portName;
    std::atomic<bool> m_running{false};

    // Debounce: 2 consecutive reads at ~500µs = ~1ms
    static constexpr int DEBOUNCE_COUNT = 2;

    // Native port handle
#ifdef Q_OS_WIN
    void *m_handle = nullptr; // HANDLE
#else
    int m_fd = -1;
#endif
};

#endif // HALIKEYWORKER_H
