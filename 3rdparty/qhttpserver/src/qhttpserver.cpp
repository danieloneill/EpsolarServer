/*
 * Copyright 2011-2014 Nikhil Marathe <nsm.nikhil@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "qhttpserver.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QVariant>
#include <QDebug>
#include <QEventLoop>

#include "qhttpconnection.h"

#ifndef qInfo
#define qInfo qWarning
#endif

template <typename N>
inline N max_inl(N n1, N n2) {
    return (n1 < n2) ? n2 : n1;
}



/// Construct a new multithreaded TCP Server.
/** @param parent Parent QObject for the server. */
QMtTcpServer::QMtTcpServer(QObject *parent, int maxThreads, int maxConnsPerThread, int maxPendingConnections) :
    QTcpServer(parent),
    m_maxThreads(maxThreads), m_maxConnsPerThread(maxConnsPerThread),
    m_prefThreads(max_inl(1,maxThreads/maxConnsPerThread))
{

    setMaxPendingConnections(maxPendingConnections);
}

void QMtTcpServer::incomingConnection(qintptr socketDescriptor) {

    // WARN newConnection has emitted from main thread prior to this current call.
    // socket and connection objects branch needs moveToThread(socket thread) and
    // Qt::DirectConnection-s of signals to make them all to receive in socket thread !

    // trivial
    //ASSERT_THREADS_MATCH(QThread::currentThread(), thread());

    auto socket = createClientSocketPeer(socketDescriptor);

    if (socket) {


        int a = activeThreads.size();

        if (a <= m_prefThreads) {


        } else {

            for (auto thread : activeThreads) {
                if (thread->add(socket)) {
                    socket->setParent(0);
                    socket->moveToThread(thread);
                    pendingSockets.push_back(new PendingSocket(thread, socket));
                    return;
                }
            }

            if (m_maxThreads <= a) {
                qWarning()<<"Too many active threads, all are full (#"<<a<<"/"<<m_maxThreads<<") : "<<socketDescriptor;
                acceptError(QAbstractSocket::ConnectionRefusedError);
                socket->abort();
                return;
            }

        }

        auto thread = new QTcpClientPeerThread(this, m_maxConnsPerThread);
        thread->add(socket);
        socket->setParent(0);
        socket->moveToThread(thread);
        activeThreads.push_back(thread);
        pendingSockets.push_back(new PendingSocket(thread,socket));

        thread->start();
    }
}

QTcpSocketL * QMtTcpServer::createClientSocketPeer(qintptr socketDescriptor) {
    // trivial
    //ASSERT_THREADS_MATCH(QThread::currentThread(), thread());

    // Affinity is (will set later to) QTcpClientPeerThread (thread),
    // so event loop is running and messaging socket (thus whole object branch)
    // from there:
    QTcpSocketL * clientPeer = new QTcpSocketL();

    int sz = pendingSockets.size();

    if (!clientPeer->setSocketDescriptor(socketDescriptor)
            //&& clientPeer->open(QTcpSocket::ReadWrite)
            ) {
        qCritical()<<"client socket failure, aborted (#"<<sz<<"/"<<maxPendingConnections()<<") : "<<socketDescriptor;
        acceptError(QAbstractSocket::SocketResourceError);
        clientPeer->abort();
        clientPeer = 0;
    } else if (maxPendingConnections()<=sz) {
        qWarning()<<"Too many pending connections (#"<<sz<<"/"<<maxPendingConnections()<<") : "<<socketDescriptor;
        acceptError(QAbstractSocket::ConnectionRefusedError);
        clientPeer->abort();
        clientPeer = 0;
    } else {
        qDebug()<<"Pending client socket accepted : (#"<<sz<<"/"<<maxPendingConnections()<<") : "<<socketDescriptor;
    }
    return clientPeer;
}

bool QMtTcpServer::hasPendingConnections() {
    ASSERT_THREADS_MATCH(QThread::currentThread(), thread());

    return pendingSockets.size();
}

QTcpSocket * QMtTcpServer::nextPendingConnection() {
    ASSERT_THREADS_MATCH(QThread::currentThread(), thread());

    if (pendingSockets.size()) {
        auto e1 = pendingSockets.first();
        pendingSockets.pop_front();
        auto r = e1->socket;
        delete e1;
        return r;
    } else {
        return 0;
    }
}



QHash<int, QString> STATUS_CODES;

QHttpServer::QHttpServer(QObject *parent, bool startInNewThread, int maxThreads, int maxConnsPerThread, int maxPendingConnections) :
    QObject(parent), m_serverThread(0), m_tcpServer(0), m_maxThreads(maxThreads), m_maxConnsPerThread(maxConnsPerThread),
    m_maxPendingConnections(maxPendingConnections)
{
/*
    if (startInNewThread) {
        if (parent) {
            qWarning() << "QHttpServer:  Cannot set QObject parent with startInNewThread :" << (void*)parent;
        }
        setParent(0);
        //m_serverThread = new QHttpServerThread(this);
        moveToThread(m_serverThread);
        m_serverThread->start();
        qInfo() << "Http Server thread started:" << (void*)m_serverThread<<"  current thread :" << (void*)QThread::currentThread();

        // NOTE this just serves the purpose moving from one thread to the other :
        connect(this, &QHttpServer::sign_listen, this, &QHttpServer::slot_listen, Qt::QueuedConnection);
    } else {
        qInfo() << "Http Server instantiated in main thread :" << (void*)QThread::currentThread();

        connect(this, &QHttpServer::sign_listen, this, &QHttpServer::slot_listen, Qt::DirectConnection);
    }
*/
        connect(this, &QHttpServer::sign_listen, this, &QHttpServer::slot_listen, Qt::DirectConnection);

#define STATUS_CODE(num, reason) STATUS_CODES.insert(num, reason);
    // {{{
    STATUS_CODE(100, "Continue")
    STATUS_CODE(101, "Switching Protocols")
    STATUS_CODE(102, "Processing") // RFC 2518) obsoleted by RFC 4918
    STATUS_CODE(200, "OK")
    STATUS_CODE(201, "Created")
    STATUS_CODE(202, "Accepted")
    STATUS_CODE(203, "Non-Authoritative Information")
    STATUS_CODE(204, "No Content")
    STATUS_CODE(205, "Reset Content")
    STATUS_CODE(206, "Partial Content")
    STATUS_CODE(207, "Multi-Status") // RFC 4918
    STATUS_CODE(300, "Multiple Choices")
    STATUS_CODE(301, "Moved Permanently")
    STATUS_CODE(302, "Moved Temporarily")
    STATUS_CODE(303, "See Other")
    STATUS_CODE(304, "Not Modified")
    STATUS_CODE(305, "Use Proxy")
    STATUS_CODE(307, "Temporary Redirect")
    STATUS_CODE(400, "Bad Request")
    STATUS_CODE(401, "Unauthorized")
    STATUS_CODE(402, "Payment Required")
    STATUS_CODE(403, "Forbidden")
    STATUS_CODE(404, "Not Found")
    STATUS_CODE(405, "Method Not Allowed")
    STATUS_CODE(406, "Not Acceptable")
    STATUS_CODE(407, "Proxy Authentication Required")
    STATUS_CODE(408, "Request Time-out")
    STATUS_CODE(409, "Conflict")
    STATUS_CODE(410, "Gone")
    STATUS_CODE(411, "Length Required")
    STATUS_CODE(412, "Precondition Failed")
    STATUS_CODE(413, "Request Entity Too Large")
    STATUS_CODE(414, "Request-URI Too Large")
    STATUS_CODE(415, "Unsupported Media Type")
    STATUS_CODE(416, "Requested Range Not Satisfiable")
    STATUS_CODE(417, "Expectation Failed")
    STATUS_CODE(418, "I\"m a teapot")        // RFC 2324
    STATUS_CODE(422, "Unprocessable Entity") // RFC 4918
    STATUS_CODE(423, "Locked")               // RFC 4918
    STATUS_CODE(424, "Failed Dependency")    // RFC 4918
    STATUS_CODE(425, "Unordered Collection") // RFC 4918
    STATUS_CODE(426, "Upgrade Required")     // RFC 2817
    STATUS_CODE(500, "Internal Server Error")
    STATUS_CODE(501, "Not Implemented")
    STATUS_CODE(502, "Bad Gateway")
    STATUS_CODE(503, "Service Unavailable")
    STATUS_CODE(504, "Gateway Time-out")
    STATUS_CODE(505, "HTTP Version not supported")
    STATUS_CODE(506, "Variant Also Negotiates") // RFC 2295
    STATUS_CODE(507, "Insufficient Storage")    // RFC 4918
    STATUS_CODE(509, "Bandwidth Limit Exceeded")
    STATUS_CODE(510, "Not Extended") // RFC 2774
    // }}}

}

QHttpServer::~QHttpServer()
{
    if (m_serverThread) {
        qDebug() << "QHttpServer  ~   m_serverThread->deleteLater() : " <<  (void*)m_serverThread;
        m_serverThread->deleteLater();
    } else {
        qDebug() << "QHttpServer  ~   no server thread, finished";
    }
}

bool QHttpServer::listen(const QHostAddress& address, quint16 port)
{
    emit sign_listen(address.toString(), port);
    // TODO dummy :
    return true;
}

bool QHttpServer::listen(quint16 port)
{
    return listen(QHostAddress::Any, port);
}

void QHttpServer::close()
{
    qDebug() << "QHttpServer . close";

    if (m_tcpServer)
        m_tcpServer->close();
}

void QHttpServer::_newConnection()
{
    Q_ASSERT(m_tcpServer);

    // NOTE signals posted from main thread :

    // trivial
    //ASSERT_THREADS_MATCH(QThread::currentThread(), thread());

    qDebug() << "QHttpServer . _newConnection";

    while (m_tcpServer->hasPendingConnections()) {
        QHttpConnection *connection =
            new QHttpConnection(this, m_tcpServer->nextPendingConnection());
        connect(connection, SIGNAL(newRequest(QHttpRequest *, QHttpResponse *)), this,
                SIGNAL(newRequest(QHttpRequest *, QHttpResponse *)), Qt::DirectConnection);
        connect(connection, SIGNAL(requestFinished(QHttpRequest *, QHttpResponse *)), this,
                SIGNAL(requestFinished(QHttpRequest *, QHttpResponse *)), Qt::DirectConnection);
        emit newConnection(connection);
    }
}

void QHttpServer::slot_listen(QString const & _address, quint16 port) {

    QHostAddress address(_address);

    qDebug() << "QHttpServer . (slot_)listen : " << address.toString()<<":"<<QString::number(port)<<" "<<(void*)QThread::currentThread();

    Q_ASSERT(!m_tcpServer);

    // trivial
    //ASSERT_THREADS_MATCH(QThread::currentThread(), thread());

    m_tcpServer = new QTcpServer(this);
    //m_tcpServer = new QMtTcpServer(this, m_maxThreads, m_maxConnsPerThread, m_maxPendingConnections);


    bool couldBindToPort = m_tcpServer->listen(address, port);
    if (couldBindToPort) {
        connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(_newConnection()));
    } else {
        delete m_tcpServer;
        m_tcpServer = NULL;
    }
    // TODO
    //return couldBindToPort;
}

