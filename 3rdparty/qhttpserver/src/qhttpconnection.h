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

#ifndef Q_HTTP_CONNECTION
#define Q_HTTP_CONNECTION

#include "qhttpserverapi.h"
#include "qhttpserverfwd.h"

#include <QObject>

/// @cond nodoc

class QHTTPSERVER_API QHttpConnection : public QObject
{
    friend class QHttpRequest;
    friend class QHttpResponse;

    Q_OBJECT

public:
    QHttpConnection(QHttpServer *parent, QTcpSocket *socket);
    virtual ~QHttpConnection();

    void write(const QByteArray &data, int offset=0, int len=-1);
    void write(const char * data, int offse, int len);
    void flush();
    void waitForBytesWritten();

    inline QTcpSocket const *socket() const { return m_socket; }
    inline QTcpSocket *socket() { return m_socket; }

    inline QString header(QString const & key) const {
        return m_currentHeaders[key];
    }
    inline QString const & specRequest() const {
        return m_currentSpecRequest;
    }
    void finishRequest();

Q_SIGNALS:
    void newRequest(QHttpRequest *, QHttpResponse *);
    void requestFinished(QHttpRequest *request, QHttpResponse *response);
    void allBytesWritten();

private Q_SLOTS:
    void parseRequest();
    void responseDone();
    void socketDisconnected();
    void invalidateRequest();
    void updateWriteCount(qint64);

private:
    static int MessageBegin(http_parser *parser);
    static int Protocol(http_parser *parser, const char *at, size_t length);
    static int Url(http_parser *parser, const char *at, size_t length);
    static int SpecRequest(http_parser *parser, const char *at, size_t length);
    static int HeaderField(http_parser *parser, const char *at, size_t length);
    static int HeaderValue(http_parser *parser, const char *at, size_t length);
    static int HeadersComplete(http_parser *parser);
    static int Body(http_parser *parser, const char *at, size_t length);
    static int MessageComplete(http_parser *parser);

private:
    QTcpSocket *m_socket;
    http_parser *m_parser;
    http_parser_settings *m_parserSettings;

    // Since there can only be one request at any time even with pipelining.
    QHttpRequest *m_request;
    QHttpResponse *m_response;

    QString m_protocol;
    QByteArray m_currentUrl;
    QString m_currentSpecRequest;
    // The ones we are reading in from the parser
    HeaderHash m_currentHeaders;
    QString m_currentHeaderField;
    QString m_currentHeaderValue;

    // Keep track of transmit buffer status
    qint64 m_transmitLen;
    qint64 m_transmitPos;

    bool m_requestFinished;
};

/// @endcond

#endif
