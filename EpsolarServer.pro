# --------
# Options:
# --------

# 1. Uncomment to use libmodbus, otherwise use QSerialBus:
CONFIG += libmodbus

# 2. If you want to use the integrated HTTP server:
CONFIG += http

# 3. If you want to use the integrated websocket server:
CONFIG += websocket

# 4. Path to configuration file:
DEFINES += SETTINGS=\"\\\"/etc/epsolarServer.conf\\\"\"

# --------

QT += core
QT -= gui
QT += sql

DEFINES += COMPILETIME=\"\\\"$${_DATE_}\\\"\"
CONFIG += c++11

TARGET = EpsolarServer
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += src/main.cpp \
    src/epsolar.cpp \
    src/controller.cpp \
    src/gzip.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    src/epsolar.h \
    src/controller.h \
    src/gzip.h

http {
    DEFINES += HTTP
    SOURCES += src/resourceserver.cpp
    HEADERS += src/resourceserver.h
    LIBS += -lqhttpserver
    RESOURCES += www/resources.qrc
}

websocket {
    QT += websockets
    HEADERS += src/websocketserver.h
    SOURCES += src/websocketserver.cpp
    DEFINES += WEBSOCKET
}

libmodbus {
    CONFIG += link_pkgconfig
    PKGCONFIG += libmodbus
    DEFINES += LIBMB
} else {
    QT += serialbus serialport
}
