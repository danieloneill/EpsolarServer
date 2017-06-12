TARGET = greeting

QT += network
QT -= gui

CONFIG += c++11
#CONFIG += debug c++11

INCLUDEPATH += ../../src
LIBS += -L../../lib

win32 {
    debug: LIBS += -lqhttpserverd
    else: LIBS += -lqhttpserver
} else {
    LIBS += -lqhttpserver
}

SOURCES = greeting.cpp
HEADERS = greeting.h
