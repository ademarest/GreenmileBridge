#ifndef CENSUSGEOCODE_H
#define CENSUSGEOCODE_H

#include "Bridge/Internet/httpconn.h"

class CensusGeocode : public HTTPConn
{
    Q_OBJECT
public:
    explicit CensusGeocode(const QString &databaseName, QObject *parent = nullptr);

    explicit CensusGeocode(const QString &databaseName,
                           const QString &serverAddress,
                           const QString &username,
                           const QString &password,
                           const QStringList &headers,
                           const int connectionFreqMS,
                           const int maxActiveConnections,
                           QObject *parent = nullptr);

    virtual ~CensusGeocode(){qDebug() << "~CGD";}

    void geocodeLocation(const QString &key,
                         QString address1,
                         QString address2,
                         QString city,
                         QString state,
                         QString zipCode);
};

#endif // CENSUSGEOCODE_H
