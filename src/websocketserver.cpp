#include "websocketserver.h"

#include <QWebSocket>

WebsocketServer::WebsocketServer(QObject *parent) : QObject(parent)
{
    m_server = new QWebSocketServer("Epsolar", QWebSocketServer::NonSecureMode, this);

    connect( m_server, &QWebSocketServer::newConnection, this, &WebsocketServer::incoming );
    m_server->listen(QHostAddress::Any, 7175);
}

void WebsocketServer::incoming()
{
    QWebSocket *socket = m_server->nextPendingConnection();

    connect( socket, &QWebSocket::disconnected, this, &WebsocketServer::disconnected );
    m_clients.append(socket);

    emit newConnection(socket);
}

void WebsocketServer::disconnected()
{
    QWebSocket *client = qobject_cast< QWebSocket * >( sender() );
    if( m_clients.contains(client) )
        m_clients.removeOne(client);

    client->deleteLater();
}

int WebsocketServer::broadcast(const QString &packet)
{
    //printf("JSON: %s\n\n", packet.toStdString().c_str());
    int received = 0;
    foreach( QWebSocket *socket, m_clients )
    {
        socket->sendTextMessage(packet);
        received++;
    }
    return received;
}
