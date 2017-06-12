#ifndef EPSOLAR_H
#define EPSOLAR_H

#include <QObject>
#include <QVariant>

#ifdef LIBMB
# include <modbus.h>
#else
# include <QtSerialBus>
# include <QtSerialPort>
#endif

class Epsolar : public QObject
{
    Q_OBJECT
public:
    explicit Epsolar(QObject *parent = 0);
    ~Epsolar();

#ifdef LIBMB
    modbus_t               *m_ctx;
#else
    QModbusRtuSerialMaster  m_client;

    QString                 m_portName;
    QSerialPort::Parity     m_parity;
    QSerialPort::BaudRate   m_baudRate;
    QSerialPort::DataBits   m_dataBits;
    QSerialPort::StopBits   m_stopBits;
    int                     m_networkPort;
    QString                 m_networkAddress;
#endif

public:
    Epsolar();

signals:
    void registerResult(quint16 reg, QVariantList values);

public slots:
    bool open(const QString &portName, quint32 baud, int data, const QString &parity, int stop );
    bool open(const QString &netAddr, int netPort );
    bool readRegister(qint32 req);

    QString errorString();
#ifndef LIBMB
private slots:
    void replyReceived();
#endif
};

#endif // EPSOLAR_H
