#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>

class WebsocketServer : public QObject
{
    Q_OBJECT

    QWebSocketServer    *m_server;

    QList< QWebSocket * >   m_clients;

public:
    explicit WebsocketServer(QObject *parent = 0);

signals:
    void newConnection( QWebSocket *socket );

public slots:
    int broadcast(const QString &packet);

private slots:
    void incoming();
    void disconnected();
};

#endif // WEBSOCKETSERVER_H
