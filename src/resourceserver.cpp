#include "resourceserver.h"

#include <qhttpserver.h>
#include <qhttprequest.h>
#include <qhttpresponse.h>

#include <QDateTime>
#include <QResource>
#include <QTimeZone>

ResourceServer::ResourceServer(QObject *parent) : QObject(parent)
{
    m_server = new QHttpServer(this);
    connect(m_server, &QHttpServer::newRequest, this, &ResourceServer::handleRequest);

    m_server->listen(QHostAddress::Any, 8080);
}

void ResourceServer::handleRequest(QHttpRequest *req, QHttpResponse *res)
{
    qDebug() << "Requested: " << req->path();
    if( req->path() == "/" )
    {
        sendResource(res, "index.html", req);
    } else if( req->path().length() > 1 ){
        QString url = req->path().mid(1);
        sendResource(res, req->path(), req);
    } else
        sendNotPermitted(res, req->path());
}

void ResourceServer::sendFourOhFour(QHttpResponse *res, const QString &path)
{
    Q_UNUSED(path)

    QByteArray data("File not found.");
    res->setHeader("Content-Type", "text/plain");
    res->setHeader("Content-Length", QString::number(data.size()));
    res->writeHead(404);
    res->end(data);
}

void ResourceServer::sendNotPermitted(QHttpResponse *res, const QString &path)
{
    Q_UNUSED(path)

    QByteArray data("Directory listing not implemented.");
    res->setHeader("Content-Type", "text/plain");
    res->setHeader("Content-Length", QString::number(data.size()));
    res->writeHead(403);
    res->end(data);
}

void ResourceServer::sendResource(QHttpResponse *res, const QString &path, QHttpRequest *req)
{
    QResource resource(path);
    if( !resource.isValid() )
        return sendFourOhFour(res, path);

    bool modified = true;
    if( !req->header("If-Modified-Since").isEmpty() )
    {
        QDateTime ifsince = fromHTTPDate(req->header("If-Modified-Since"));
        QDateTime now = compileTime();
        qDebug() << "IF MODIFIED SINCE: " << ifsince << " < " << now;

        if( ifsince <= now )
            modified = false;
    }

    //if( !resource.children().isEmpty() )
        //return sendNotPermitted(res, path);

    QByteArray data;
    if( modified )
    {
        if( resource.isCompressed() )
            data = qUncompress( QByteArray( (const char *)resource.data(), resource.size() ) );
        else
            data = QByteArray( (const char *)resource.data(), resource.size() );

        res->setHeader("Content-Length", QString::number(data.size()));
    }

    res->setHeader("Last-Modified", toHTTPDate(compileTime()) );

    // Mimetype?
    if( path.endsWith(".html") )
        res->setHeader("Content-Type", "text/html");
    else if( path.endsWith(".js") )
        res->setHeader("Content-Type", "text/javascript");
    else if( path.endsWith(".css") )
        res->setHeader("Content-Type", "text/css");

    if( modified )
    {
        res->writeHead(200);
        res->end(data);
    } else {
        res->writeHead(304);
        res->end();
    }
}

QDateTime ResourceServer::compileTime()
{
    QDateTime result = QDateTime::fromString(QString(COMPILETIME), "ddd MMM dd HH:mm:ss yyyy");
    return result;
}

QDateTime ResourceServer::fromHTTPDate(const QString &httpdate)
{
    QDateTime result = QDateTime::fromString(httpdate.mid(5, httpdate.length() - 9), "dd MMM yyyy HH:mm:ss");
    QTimeZone utc("UTC");
    result.setTimeZone(utc);
    return result.toLocalTime();
}

QString ResourceServer::toHTTPDate(QDateTime datetime)
{
    return datetime.toUTC().toString("ddd, dd MMM yyyy HH:mm:ss").append(" GMT");
}
