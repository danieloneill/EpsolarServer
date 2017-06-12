#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QDateTime>
#include <QObject>
#include <QSettings>
#include <QVariantMap>
#include <QTimer>

#ifdef WEBSOCKET
#include <QJsonObject>
#endif

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

class Epsolar;
#ifdef WEBSOCKET
class WebsocketServer;
class QWebSocket;
#endif
class Controller : public QObject
{
    Q_OBJECT

    QTimer          m_timer;
    int             m_index;
    QVariantMap     m_values;

    QDateTime       m_lastAverage;
    QMap< quint16, QList< double > > m_averages;
    QMap< quint16, QList< QPair< QDateTime, qreal > > > m_readings;

#ifdef WEBSOCKET
    QList< QWebSocket * > m_connections;
    WebsocketServer *m_wss;
#endif

    Epsolar         *m_epsolar;
    QSqlDatabase    m_db;

    bool loadRegisters();
    void addRegister(quint16 reg, const QString &name, double scale=0, int lowhigh=0);

    void addAverages();
    void saveAverages();
    void clearAverages();

    void compressHourly();
    void trimForHourly();

    void sendValues();
    void mapBits();

    void addReadings();
    void trimReadings();

#ifdef WEBSOCKET
    QJsonObject loadAverages(const QDateTime &from, const QDateTime &to, quint16 reg=0, quint32 count=120);
    QJsonObject loadHourly(const QDateTime &from, const QDateTime &to, quint16 reg=0, quint32 count=120);
    QJsonObject loadReadings(quint32 count=1000);
    void sendAverages(QWebSocket *socket, const QDateTime &from, const QDateTime &to, quint16 reg=0, quint32 count=1000);
    void sendHourly(QWebSocket *socket, const QDateTime &from, const QDateTime &to, quint16 reg=0, quint32 count=1000);
    void sendLatest(QWebSocket *socket, quint32 count=1000);
#endif
public:
    explicit Controller(QSettings *settings, QObject *parent = 0);

signals:

private slots:
    void timerTriggered();
#ifdef WEBSOCKET
    void handleConnection( QWebSocket *socket );
    void handleDisconnect();
    void handlePacket(const QString &message);
#endif
    void registerReceived(quint16 register reg, QVariantList values);
};

#endif // CONTROLLER_H
