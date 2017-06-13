#include "controller.h"
#include "epsolar.h"
#include "websocketserver.h"
#include "gzip.h"

#ifdef WEBSOCKET
# include <QJsonArray>
# include <QJsonDocument>
# include <QJsonValue>
# include <QJsonParseError>
#endif

#include <QCoreApplication>

#include <QWebSocket>

#define LOW 1
#define HIGH 2

QHash<quint16, QVariantMap> v_registers;
QList<quint16> l_registers;

Controller::Controller(QSettings *settings, QObject *parent) : QObject(parent),
    m_index(0)
{
#ifdef WEBSOCKET
    m_wss = new WebsocketServer(this);
    connect( m_wss, &WebsocketServer::newConnection, this, &Controller::handleConnection );
#endif

    m_db = QSqlDatabase::addDatabase(settings->value("databaseType", "QMYSQL").toString());
    m_db.setDatabaseName(settings->value("databaseName", "epsolar").toString());
    m_db.setHostName(settings->value("databaseHostname", "localhost").toString());
    m_db.setUserName(settings->value("databaseUsername", "root").toString());
    m_db.setPassword(settings->value("databasePassword", "").toString());
    if( !m_db.open() )
    {
        qWarning() << "Connection failed: " << m_db.lastError();
        qApp->exit(1);
        return;
    }

    m_lastAverage = QDateTime::currentDateTime();

    m_epsolar = new Epsolar(this);
    QString devPath = settings->value("epsolarDevicePath", "/dev/ttyXRUSB0").toString();
    if( !m_epsolar->open(devPath, 115200, 8, "N", 1) )
    {
        qDebug() << "Failed to open " << devPath << m_epsolar->errorString();
        return;
    }
    connect( m_epsolar, &Epsolar::registerResult, this, &Controller::registerReceived );

    loadRegisters();

    m_timer.setInterval(settings->value("epsolarPollFrequencyMS", 50).toInt());
    m_timer.setSingleShot(false);
    connect( &m_timer, &QTimer::timeout, this, &Controller::timerTriggered );
    m_timer.start();
}

void Controller::addRegister(quint16 reg, const QString &name, double scale, int lowhigh)
{
    QVariantMap val;
    val["n"] = name;
    if( scale != 0 )
        val["scale"] = scale;
    if( lowhigh != 0 )
        val["lowhigh"] = lowhigh;
    v_registers[reg] = val;
}

bool Controller::loadRegisters()
{
    QSqlQuery query(m_db);
    query.prepare("SELECT register, name, measure, scale, multibyte FROM registers ORDER BY id");
    if( !query.exec() )
        return false;

    while( query.next() )
    {
        quint16 reg = query.value(0).toInt();
        QString name = query.value(1).toString();
        QString measure = query.value(2).toString();
        qreal scale = query.value(3).toReal();
        QString multibyte = query.value(4).toString();

        int lh = 0;
        if( multibyte == "LOW" ) lh = LOW;
        else if( multibyte == "HIGH" ) lh = HIGH;

        addRegister( reg, name, scale, lh );
    }

    QList<quint16> regs = v_registers.keys();
    foreach( quint16 key, regs )
        l_registers.append(key);

    qDebug() << "Loaded registers: " << v_registers;

    return true;
}

void Controller::addAverages()
{
    QDateTime now = QDateTime::currentDateTime();
    foreach( quint16 reg, v_registers.keys() )
    {
        QString key = v_registers[reg]["n"].toString();
        m_averages[ reg ].append( m_values[key].toDouble() );
    }

    if( m_lastAverage.secsTo(now) >= 300 )
    {
        saveAverages();
        clearAverages();

        // Calculate the hourly?
        if( m_lastAverage.time().hour() != now.time().hour() )
        {
            // Compress old (pre-24-hours-ago) readings into houry table:
            compressHourly();
            trimForHourly();
        }
        m_lastAverage = now;
    }
}

void Controller::clearAverages()
{
    m_averages.clear();
    // ... okay, that's simple enough.
}

void Controller::saveAverages()
{
    if( !m_db.transaction() )
    {
        qWarning() << "Failed to open an averages transaction: " << m_db.lastError();
        return;
    }

    bool success = true;
    foreach( quint16 reg, m_averages.keys() )
    {
        double min=0, max=0, avg=0;
        QDateTime now = QDateTime::currentDateTime();

        quint16 listLen = m_averages[reg].length();
        for( quint16 x=0; x < listLen; x++ )
        {
            double val = m_averages[reg][x];
            if( val < min || x == 0 ) min = val;
            if( val > max || x == 0 ) max = val;
            avg += ( val / listLen );
        }

        QSqlQuery query(m_db);
        if( !query.prepare("INSERT INTO fiveMinute(register, min, max, average, tstart, tend)VALUES(?, ?, ?, ?, ?, ?)") )
        {
            success = false;
            break;
        }

        query.addBindValue(reg);
        query.addBindValue(min);
        query.addBindValue(max);
        query.addBindValue(avg);
        query.addBindValue(m_lastAverage);
        query.addBindValue(now);
        if( !query.exec() )
        {
            success = false;
            break;
        }
    }

    if( success )
        m_db.commit();
    else
    {
        qWarning() << "Transaction failed: " << m_db.lastError();
        m_db.rollback();
        return;
    }
}

void Controller::addReadings()
{
    QDateTime whence = QDateTime::currentDateTime();
    foreach( quint16 reg, v_registers.keys() )
    {
        QString key = v_registers[reg]["n"].toString();
        QPair< QDateTime, qreal > reading;

        reading.first = whence;
        reading.second = m_values[key].toReal();

        m_readings[reg].append(reading);
    }
}

void Controller::trimReadings()
{
    foreach( quint16 key, m_readings.keys() )
    {
        while( m_readings[key].length() > 8000 )
            m_readings[key].removeFirst();
    }
}

void Controller::mapBits()
{
    union u_pair {
        struct {
                quint16 low;
                quint16 high;
        } hl;
        quint32 u32;
    };

    QMap<QString, quint32> lowHighs;
    foreach( QString key, m_values.keys() )
    {
        bool exists = false;
        union u_pair mapped;
        QString nakedKey = key.mid( 0, key.length()-2 );
        if( lowHighs.contains(nakedKey) )
        {
                mapped.u32 = lowHighs[nakedKey];
                exists = true;
        }
        if( key.endsWith(":L") )
        {
                quint16 lowValue = m_values[key].toInt();
                mapped.hl.low = lowValue;
                m_values.remove(key);
                if( exists )
                        m_values[nakedKey] = mapped.u32;
                else
                        lowHighs[nakedKey] = mapped.u32;
        }
        else if( key.endsWith(":H") )
        {
                quint16 highValue = m_values[key].toInt();
                mapped.hl.high = highValue;
                m_values.remove(key);
                if( exists )
                        m_values[nakedKey] = mapped.u32;
                else
                        lowHighs[nakedKey] = mapped.u32;
        }
    }
}

void Controller::compressHourly()
{
    QSqlQuery query(m_db);

    QString queryStr = QString("INSERT INTO hourly(register, min, max, average, tstart, tend) SELECT register, MIN(min) AS min, MAX(max) AS max, AVG(average) AS average, MIN(tstart) AS tstart, MAX(tend) AS tend FROM fiveMinute WHERE tstart < SUBTIME(CONCAT(MAKEDATE(YEAR(now()), DAYOFYEAR(now())),' ',MAKETIME(HOUR(now()),0,0)), '24:00:00.000000') GROUP BY YEAR(tstart), MONTH(tstart), DAY(tstart), HOUR(tstart), register");
    query.exec(queryStr);
}

void Controller::trimForHourly()
{
    QSqlQuery query(m_db);

    QString queryStr = QString("DELETE FROM fiveMinute WHERE tstart < SUBTIME(now(), '24:00:00.000000')");
    query.exec(queryStr);
}

void Controller::sendValues()
{
    // Convert LOW/HIGH to m_value for >16-bit values:
    mapBits();

    addAverages();
    trimReadings();
    addReadings();

#ifdef WEBSOCKET
    QVariantMap obj;
    obj["type"] = "reading";
    obj["data"] = m_values;
    QJsonDocument doc = QJsonDocument::fromVariant(obj);
    QString asStr = QString( doc.toJson() );
    //qDebug() << "Json: " << asStr;

    QByteArray binary;
    foreach( Connection *client, m_connections )
    {
        if( !client->m_subscribed ) continue;

        if( client->m_compressed )
        {
            // Only compress if 1+ clients are using compression, and then cache it.
            if( binary.isEmpty() )
                binary = GZip::compress(asStr.toUtf8());

            client->m_client->sendBinaryMessage( binary );
        }
        else
            client->m_client->sendTextMessage(asStr);
    }
#endif
}

void Controller::timerTriggered()
{
    if( l_registers.length() <= m_index )
        return;

    quint16 reg = l_registers[m_index];
    //QVariantMap values = v_registers[reg];
    //qDebug() << "Reading register: " << values["n"].toString();
    m_epsolar->readRegister( reg );
}

void Controller::registerReceived(quint16 reg, QVariantList values)
{
    if( values.length() < 1 )
        return;

    QVariantMap ent = v_registers[reg];
    QString regName = ent["n"].toString();

    QVariant value = values[0];
    if( ent.contains("scale") )
    {
        double scale = ent["scale"].toDouble();
        value = (double)(values[0].toInt() * scale);
    }
    if( ent.contains("lowhigh") )
    {
        int lowhigh = ent["lowhigh"].toInt();
        if( lowhigh == LOW )
            regName.append(":L");
        else
            regName.append(":H");
    }
    m_values[regName] = value;

    m_index++;
    if( m_index >= l_registers.length() )
    {
        // All registers filled, transmit!
        sendValues();
        m_index = 0;
    }
}

#ifdef WEBSOCKET
Connection *Controller::mapConnection( QWebSocket *socket )
{
    foreach( Connection *conn, m_connections )
        if( conn->m_client == socket )
            return conn;
    return nullptr;
}

void Controller::handleConnection(QWebSocket *socket)
{
    connect( socket, &QWebSocket::textMessageReceived, this, &Controller::handlePacket );
    connect( socket, &QWebSocket::disconnected, this, &Controller::handleDisconnect );

    Connection *conn = new Connection(this);
    conn->m_client = socket;
    conn->m_compressed = false;
    conn->m_subscribed = false;
    m_connections.push_back(conn);
}

void Controller::handleDisconnect()
{
    QWebSocket *socket = qobject_cast< QWebSocket * >( sender() );
    Connection *conn = mapConnection(socket);
    if( conn )
        m_connections.removeOne( conn );
    socket->deleteLater();
}

void Controller::handlePacket(const QString &message)
{
    QWebSocket *socket = qobject_cast< QWebSocket * >( sender() );
    Connection *conn = mapConnection(socket);
    if( !conn )
    {
        socket->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());

    if( !doc.isObject() ) return;
    QJsonObject obj = doc.object();

    if( !obj.contains("action") ) return;

    if( obj.contains("compress") )
        conn->m_compressed = obj.value("compress").toBool();

    if( obj.value("action").toString() == "latest" )
    {
        quint32 count = 1000;
        if( obj.contains("count") )
            count = obj.value("count").toInt(1000);

        return sendLatest(socket, count);
    }
    else if( obj.value("action").toString() == "averages" )
    {
        quint32 count = 1000;
        if( obj.contains("count") )
            count = obj.value("count").toInt(1000);

        quint16 reg = 0;
        if( obj.contains("register") )
            reg = obj.value("register").toInt();

        if( !obj.contains("from") ||!obj.contains("to") )
            return;

        QDateTime from = QDateTime::fromMSecsSinceEpoch( obj.value("from").toVariant().toULongLong() );
        QDateTime to = QDateTime::fromMSecsSinceEpoch( obj.value("to").toVariant().toULongLong() );

        return sendAverages(socket, from, to, reg, count);
    }
    else if( obj.value("action").toString() == "hourly" )
    {
        quint32 count = 1000;
        if( obj.contains("count") )
            count = obj.value("count").toInt(1000);

        quint16 reg = 0;
        if( obj.contains("register") )
            reg = obj.value("register").toInt();

        if( !obj.contains("from") ||!obj.contains("to") )
            return;

        QDateTime from = QDateTime::fromMSecsSinceEpoch( obj.value("from").toVariant().toULongLong() );
        QDateTime to = QDateTime::fromMSecsSinceEpoch( obj.value("to").toVariant().toULongLong() );

        return sendHourly(socket, from, to, reg, count);
    }
    else if( obj.value("action").toString() == "subscribe" )
    {
        bool onoff = true;
        if( obj.contains("subscribe") )
            onoff = obj.value("subscribe").toBool();

        conn->m_subscribed = onoff;
    }
}

void Controller::sendAverages(QWebSocket *socket, const QDateTime &from, const QDateTime &to, quint16 reg, quint32 count)
{
    Connection *conn = mapConnection(socket);
    if( !conn )
    {
        socket->deleteLater();
        return;
    }

    QJsonObject obj = loadAverages(from, to, reg, count);
    QJsonObject pkt;
    pkt.insert("type", QJsonValue("averages"));
    pkt.insert("data", QJsonValue(obj));
    QJsonDocument doc = QJsonDocument( pkt );
    QString asStr = QString( doc.toJson() );
    //qDebug() << "Json: " << asStr;

    if( conn->m_compressed )
        socket->sendBinaryMessage( GZip::compress(asStr.toUtf8()) );
    else
        socket->sendTextMessage(asStr);
}

void Controller::sendHourly(QWebSocket *socket, const QDateTime &from, const QDateTime &to, quint16 reg, quint32 count)
{
    Connection *conn = mapConnection(socket);
    if( !conn )
    {
        socket->deleteLater();
        return;
    }

    QJsonObject obj = loadHourly(from, to, reg, count);
    QJsonObject pkt;
    pkt.insert("type", QJsonValue("hourly"));
    pkt.insert("data", QJsonValue(obj));
    QJsonDocument doc = QJsonDocument( pkt );
    QString asStr = QString( doc.toJson() );
    //qDebug() << "Json: " << asStr;

    if( conn->m_compressed )
        socket->sendBinaryMessage( GZip::compress(asStr.toUtf8()) );
    else
        socket->sendTextMessage(asStr);
}

void Controller::sendLatest(QWebSocket *socket, quint32 count)
{
    Connection *conn = mapConnection(socket);
    if( !conn )
    {
        socket->deleteLater();
        return;
    }

    QJsonObject obj = loadReadings(count);
    QJsonObject pkt;
    pkt.insert("type", QJsonValue("latest"));
    pkt.insert("data", QJsonValue(obj));
    QJsonDocument doc = QJsonDocument( pkt );
    QString asStr = QString( doc.toJson() );
    //qDebug() << "Json: " << asStr;

    if( conn->m_compressed )
        socket->sendBinaryMessage( GZip::compress(asStr.toUtf8()) );
    else
        socket->sendTextMessage(asStr);
}

QJsonObject Controller::loadHourly(const QDateTime &from, const QDateTime &to, quint16 reg, quint32 count)
{
    QJsonObject jsmap;
    quint32 limit = 8000;
    if( count < limit )
        limit = count;

    QSqlQuery query(m_db);
    QString args;
    if( reg > 0 )
        args = " AND register=" + QString::number(reg);

    QString queryStr = QString("SELECT register, min, max, average, DATE(tstart) AS `date`, HOUR(tstart) AS `hour` FROM hourly WHERE tstart >= ? AND tend <= ? %1 ORDER BY register, tstart LIMIT ?").arg(args);
    if( !query.prepare(queryStr) )
    {
        return jsmap;
    }

    query.addBindValue(from);
    query.addBindValue(to);
    query.addBindValue(limit);
    if( !query.exec() )
    {
        return jsmap;
    }

    QMap< quint16, QJsonArray > ents;
    while( query.next() )
    {
        quint16 reg = query.value(0).toInt();
        qreal min = query.value(1).toReal();
        qreal max = query.value(2).toReal();
        qreal avg = query.value(3).toReal();
        QString date = query.value(4).toString();
        quint8 hour = query.value(5).toInt();

        QJsonObject entry;
        entry.insert("min", min);
        entry.insert("max", max);
        entry.insert("avg", avg);
        entry.insert("date", date);
        entry.insert("hour", hour);

        ents[ reg ].append(QJsonValue(entry));
    }

    foreach( quint16 reg, ents.keys() )
    {
        QVariantMap regent = v_registers[reg];
        QString regName = regent["n"].toString();

        jsmap.insert( regName, ents[reg] );
    }

    return jsmap;
}

QJsonObject Controller::loadReadings(quint32 count)
{
    QJsonObject jsmap;

    foreach( quint16 reg, m_readings.keys() )
    {
        QJsonArray vallist;
        quint32 rcount = m_readings[reg].count();
        for( quint32 x=0; x < rcount && x < count; x++ )
        {
            int pos = rcount - (x+1);
            QPair< QDateTime, qreal > ent = m_readings[reg][pos];
            QJsonObject pair;
            pair.insert("whence", ent.first.toMSecsSinceEpoch());
            pair.insert("value", ent.second);
            vallist.prepend(QJsonValue(pair));
        }

        QVariantMap regent = v_registers[reg];
        QString regName = regent["n"].toString();

        jsmap.insert( regName, vallist );
    }

    return jsmap;
}

QJsonObject Controller::loadAverages(const QDateTime &from, const QDateTime &to, quint16 reg, quint32 count)
{
    QJsonObject jsmap;
    quint32 limit = 8000;
    if( count < limit )
        limit = count;

    QSqlQuery query(m_db);
    QString args;
    if( reg > 0 )
        args = " AND register=" + QString::number(reg);

    QString queryStr = QString("SELECT register, min, max, average, tstart, tend FROM fiveMinute WHERE tstart >= ? AND tend <= ? %1 ORDER BY register, tstart LIMIT ?").arg(args);
    if( !query.prepare(queryStr) )
    {
        return jsmap;
    }

    query.addBindValue(from);
    query.addBindValue(to);
    query.addBindValue(limit);
    if( !query.exec() )
    {
        return jsmap;
    }

    QMap< quint16, QJsonArray > ents;
    while( query.next() )
    {
        quint16 reg = query.value(0).toInt();
        qreal min = query.value(1).toReal();
        qreal max = query.value(2).toReal();
        qreal avg = query.value(3).toReal();
        QDateTime start = query.value(4).toDateTime();
        QDateTime end = query.value(5).toDateTime();

        QJsonObject entry;
        entry.insert("min", min);
        entry.insert("max", max);
        entry.insert("avg", avg);
        entry.insert("start", start.toMSecsSinceEpoch());
        entry.insert("end", end.toMSecsSinceEpoch());

        ents[ reg ].append(QJsonValue(entry));
    }

    foreach( quint16 reg, ents.keys() )
    {
        QVariantMap regent = v_registers[reg];
        QString regName = regent["n"].toString();

        jsmap.insert( regName, ents[reg] );
    }

    return jsmap;
}

#endif
