TARGET = bodydata

QT += network
QT -= gui

CONFIG += debug c++11

INCLUDEPATH += ../../src
LIBS += -L../../lib

win32 {
    debug: LIBS += -lqhttpserverd
    else: LIBS += -lqhttpserver
} else {
    LIBS += -lqhttpserver
}

SOURCES = bodydata.cpp
HEADERS = bodydata.h
