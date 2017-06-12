#include "epsolar.h"

#include <QDebug>

Epsolar::Epsolar(QObject *parent) : QObject(parent)
{
#ifdef LIBMB
    m_ctx = NULL;
#endif
}

Epsolar::~Epsolar()
{
#ifdef LIBMB
    modbus_close(m_ctx);
    modbus_free(m_ctx);
    m_ctx = NULL;
#endif
}

#ifdef LIBMB
bool Epsolar::open( const QString &portName, quint32 baud, int data, const QString &parity, int stop )
{
    struct timeval tv;
    if( parity.length() < 1 )
    {
        return false;
    }

    m_ctx = modbus_new_rtu(portName.toStdString().c_str(), baud, parity.toStdString().c_str()[0], data, stop);
    if( m_ctx == NULL )
    {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return false;
    }

    modbus_set_slave(m_ctx, 1);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    modbus_set_response_timeout(m_ctx, &tv);
    modbus_set_byte_timeout(m_ctx, &tv);

    if( modbus_connect(m_ctx) == -1 )
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(m_ctx);
        m_ctx = NULL;
        return false;
    }

    fprintf(stderr, "Connected\n");
    return true;
}

bool Epsolar::open( const QString &netAddr, int netPort )
{
    m_ctx = modbus_new_tcp(netAddr.toStdString().c_str(), netPort);
    if( m_ctx == NULL )
    {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return false;
    }

    modbus_set_slave(m_ctx, 1);

    if( modbus_connect(m_ctx) == -1 )
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(m_ctx);
        m_ctx = NULL;
        return false;
    }

    fprintf(stderr, "Connected\n");
    return true;
}

bool Epsolar::readRegister( qint32 req )
{
    uint16_t res[64];
    int ret = modbus_read_input_registers(m_ctx, req, 1, res);
    if( ret <= 0 )
    {
        fprintf(stderr, "%s\n", modbus_strerror(errno));
        return false;
    }

    QVariantList list;
    for( int x=0; x < ret; x++ )
        list.append( res[x] );

    emit registerResult( req, list );
    return true;
}

QString Epsolar::errorString()
{
    QString res( modbus_strerror(errno) );
    return res;
}

#else
bool Epsolar::open( const QString &portName, quint32 baud, int data, const QString &parity, int stop )
{
    // DATA BITS:
    QSerialPort::DataBits db = QSerialPort::UnknownDataBits;
    switch( data )
    {
    case 5:
        db = QSerialPort::Data5;
        break;
    case 6:
        db = QSerialPort::Data6;
        break;
    case 7:
        db = QSerialPort::Data7;
        break;
    case 8:
        db = QSerialPort::Data8;
        break;
    }

    // PARITY:
    QSerialPort::Parity p = QSerialPort::UnknownParity;
    char pchar = parity.toStdString().c_str()[0];
    switch( pchar )
    {
    case 'N':
    case 'n':
        p = QSerialPort::NoParity;
        break;
    case 'E':
    case 'e':
        p = QSerialPort::EvenParity;
        break;
    case 'O':
    case 'o':
        p = QSerialPort::OddParity;
        break;
    case 'S':
    case 's':
        p = QSerialPort::SpaceParity;
        break;
    case 'M':
    case 'm':
        p = QSerialPort::MarkParity;
        break;
    }

    // STOP BITS:
    QSerialPort::StopBits sb = QSerialPort::UnknownStopBits;
    switch( stop )
    {
    case 1:
        sb = QSerialPort::OneStop;
        break;
    case 2:
        sb = QSerialPort::TwoStop;
        break;
    case 3:
        sb = QSerialPort::OneAndHalfStop;
        break;
    }

    m_client.setConnectionParameter( QModbusClient::SerialPortNameParameter, portName );
    m_client.setConnectionParameter( QModbusClient::SerialBaudRateParameter, baud );
    m_client.setConnectionParameter( QModbusClient::SerialDataBitsParameter, db );
    m_client.setConnectionParameter( QModbusClient::SerialParityParameter, p );
    m_client.setConnectionParameter( QModbusClient::SerialStopBitsParameter, sb );

    return m_client.connectDevice();
}

bool Epsolar::open( const QString &netAddr, int netPort )
{
    m_client.setConnectionParameter( QModbusClient::NetworkAddressParameter, netAddr );
    m_client.setConnectionParameter( QModbusClient::NetworkPortParameter, netPort );
    return m_client.connectDevice();
}

bool Epsolar::readRegister( qint32 req )
{
    QModbusDataUnit rdu(QModbusDataUnit::InputRegisters, req, 1);
    int sa = 1;
    QModbusReply *reply = m_client.sendReadRequest( rdu, sa );
    if( !reply )
        return false;

    connect( reply, SIGNAL(finished()), this, SLOT(replyReceived()) );
    return true;
}

void Epsolar::replyReceived()
{
    QModbusReply *reply = qobject_cast< QModbusReply * >( sender() );
    if( reply->error() != QModbusDevice::NoError )
    {
        //qDebug() << "REPLY: " << reply->errorString();
        reply->deleteLater();
        return;
    }

    QModbusDataUnit data = reply->result();
    quint8 dataLength = data.valueCount();
    QVariantList list;

    for( quint8 x=0; x < dataLength; x++ )
        list.append( data.value(x) );

    reply->deleteLater();
    qDebug() << list;
    emit registerResult( data.startAddress(), list );
}

QString Epsolar::errorString()
{
    return m_client.errorString();
}
#endif
