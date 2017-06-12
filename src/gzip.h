#ifndef GZIP_H
#define GZIP_H

#include <QObject>

class GZip : public QObject
{
    Q_OBJECT
public:
    explicit GZip(QObject *parent = 0);

    static QByteArray compress(const QByteArray& data);
    static quint32 crc32buf(const QByteArray& data);

protected:
    static quint32 updateCRC32(quint32 crc, unsigned char ch);
};

#endif // GZIP_H
