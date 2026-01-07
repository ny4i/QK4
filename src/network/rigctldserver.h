#ifndef RIGCTLDSERVER_H
#define RIGCTLDSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class RadioState;

class RigctldServer : public QObject {
    Q_OBJECT

public:
    explicit RigctldServer(RadioState *state, QObject *parent = nullptr);
    ~RigctldServer();

    bool start(quint16 port = 4532);
    void stop();
    bool isListening() const;
    quint16 port() const;
    int clientCount() const;

signals:
    void started(quint16 port);
    void stopped();
    void clientConnected(const QString &address);
    void clientDisconnected(const QString &address);
    void frequencyRequested(quint64 freqHz);
    void modeRequested(const QString &mode, int bandwidth);
    void pttRequested(bool on);
    void splitRequested(bool enabled);
    void splitFrequencyRequested(quint64 freqHz);
    void ritRequested(int offsetHz);
    void xitRequested(int offsetHz);
    void antennaRequested(int antenna);
    void levelRequested(const QString &level, int value);
    void funcRequested(const QString &func, bool enabled);
    void errorOccurred(const QString &error);

private slots:
    void onNewConnection();
    void onClientData();
    void onClientDisconnected();

private:
    QString handleCommand(const QString &cmd);
    QString handleGetFrequency(bool extended) const;
    QString handleSetFrequency(const QString &args);
    QString handleGetMode(bool extended) const;
    QString handleSetMode(const QString &args);
    QString handleGetPtt(bool extended) const;
    QString handleSetPtt(const QString &args);
    QString handleGetVfo(bool extended) const;
    QString handleGetSplit(bool extended) const;
    QString handleSetSplit(const QString &args);
    QString handleGetSplitFreq(bool extended) const;
    QString handleSetSplitFreq(const QString &args);
    QString handleGetSplitMode(bool extended) const;
    QString handleGetRit(bool extended) const;
    QString handleSetRit(const QString &args);
    QString handleGetXit(bool extended) const;
    QString handleSetXit(const QString &args);
    QString handleGetAntenna(bool extended) const;
    QString handleSetAntenna(const QString &args);
    QString handleGetTuningStep(bool extended) const;
    QString handleGetLevel(const QString &level, bool extended) const;
    QString handleSetLevel(const QString &args);
    QString handleGetFunc(const QString &func, bool extended) const;
    QString handleSetFunc(const QString &args);
    QString handleGetVfoInfo(const QString &vfo, bool extended) const;
    QString handleGetRigInfo() const;
    QString handleDumpState() const;
    QString modeToHamlib(int mode) const;
    int hamlibToMode(const QString &mode) const;

    QTcpServer *m_server;
    RadioState *m_radioState;
    QList<QTcpSocket *> m_clients;
    QMap<QTcpSocket *, QByteArray> m_clientBuffers;
    quint16 m_port = 0;

    // Pending values cache - return these until RadioState catches up
    // This eliminates set/get mismatches that cause Hamlib retries/delays
    mutable quint64 m_pendingFrequency = 0;
    mutable qint64 m_pendingFrequencyTime = 0;
    mutable int m_pendingMode = -1;
    mutable qint64 m_pendingModeTime = 0;
    static constexpr qint64 PENDING_TIMEOUT_MS = 2000; // Use pending value for 2 seconds
};

#endif // RIGCTLDSERVER_H
