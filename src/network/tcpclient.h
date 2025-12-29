#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include "protocol.h"

class TcpClient : public QObject {
    Q_OBJECT

public:
    enum ConnectionState { Disconnected, Connecting, Authenticating, Connected };
    Q_ENUM(ConnectionState)

    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    void connectToHost(const QString &host, quint16 port, const QString &password);
    void disconnectFromHost();
    bool isConnected() const;
    ConnectionState connectionState() const;

    void sendCAT(const QString &command);
    void sendRaw(const QByteArray &data);

    Protocol *protocol() { return m_protocol; }

signals:
    void stateChanged(ConnectionState state);
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void authenticated();
    void authenticationFailed();

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onAuthTimeout();
    void onPingTimer();
    void onCatResponse(const QString &response);

private:
    void setState(ConnectionState state);
    void sendAuthentication();
    void startPingTimer();
    void stopPingTimer();

    QTcpSocket *m_socket;
    Protocol *m_protocol;
    QTimer *m_authTimer;
    QTimer *m_pingTimer;

    QString m_host;
    quint16 m_port;
    QString m_password;
    ConnectionState m_state;
    bool m_authResponseReceived;
};

#endif // TCPCLIENT_H
