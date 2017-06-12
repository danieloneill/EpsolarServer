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

#ifndef Q_HTTP_SERVER_API
#define Q_HTTP_SERVER_API

#include <QtGlobal>
#include <QString>
#include <QThread>
#include <QTcpSocket>
#include <QHostAddress>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#ifdef Q_OS_WIN
// Define to export or import depending if we are building or using the library.
// QHTTPSERVER_EXPORT should only be defined when building.
#if defined(QHTTPSERVER_EXPORT)
#define QHTTPSERVER_API __declspec(dllexport)
#else
#define QHTTPSERVER_API __declspec(dllimport)
#endif
#else
// Define empty for other platforms
#define QHTTPSERVER_API
#endif
#else
#ifdef Q_WS_WIN
// Define to export or import depending if we are building or using the library.
// QHTTPSERVER_EXPORT should only be defined when building.
#if defined(QHTTPSERVER_EXPORT)
#define QHTTPSERVER_API __declspec(dllexport)
#else
#define QHTTPSERVER_API __declspec(dllimport)
#endif
#else
// Define empty for other platforms
#define QHTTPSERVER_API
#endif
#endif

#define S1(x) #x
#define S2(x) S1(x)
#define ASSERT_THREADS_MATCH(t1, t2) assertThreadsMatch(__FILE__ ":" S2(__LINE__), #t1, t1, #t2, t2)
#define ASSERT_THREADS_DIFFERENT(t1, t2) assertThreadsDifferent(__FILE__ ":" S2(__LINE__), #t1, t1, #t2, t2)

inline QString s(QTcpSocket const & socket) {
    QString s;
    if (socket.isOpen()) {
        s = socket.peerName()+":"+socket.peerAddress().toString()+":"+QString::number(socket.peerPort())+
                "#"+QString::number((std::ptrdiff_t)socket.thread(),16);
    } else {
        s+="<socket closed>";
    }

    s+=":cur#"+QString::number((std::ptrdiff_t)QThread::currentThread(),16) ;
    return s;
}
inline void assertThreadsMatch(std::string const & loc, QString const & idt1, QThread const *t1, QString const & idt2, QThread const *t2) {
    if (t1 != t2) {
        qCritical()<<"file:///"<<loc.data()<<": "<<"Assertion failed.  Threads should be the same  :  "<<idt1<<":"<<(void*)t1<<
            " != "<<idt2<<":"<<(void*)t2 << endl;
        throw std::exception();
    }
}
inline void assertThreadsDifferent(std::string const & loc, QString const & idt1, QThread const *t1, QString const & idt2, QThread const *t2) {
    if (t1 == t2) {
        qCritical()<<"file:///"<<loc.data()<<": "<<"Assertion failed.  Threads should be different  :  "<<idt1<<":"<<(void*)t1<<
            " == "<<idt2<<":"<<(void*)t2 << endl;
        throw std::exception();
    }
}

#endif
