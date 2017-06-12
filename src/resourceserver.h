#ifndef RESOURCESERVER_H
#define RESOURCESERVER_H

#include <QObject>

class QHttpRequest;
class QHttpResponse;
class QHttpServer;
class ResourceServer : public QObject
{
    Q_OBJECT

    QHttpServer     *m_server;

    QDateTime compileTime();
    QDateTime fromHTTPDate(const QString &httpdate);
    QString toHTTPDate(QDateTime datetime);

public:
    explicit ResourceServer(QObject *parent = 0);

    void sendResource(QHttpResponse *res, const QString &path, QHttpRequest *req);
    void sendFourOhFour(QHttpResponse *res, const QString &path);
    void sendNotPermitted(QHttpResponse *res, const QString &path);

signals:

public slots:
    void handleRequest(QHttpRequest *req, QHttpResponse *res);
};

#endif // RESOURCESERVER_H
