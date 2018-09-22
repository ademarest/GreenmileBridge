#ifndef ARCGISGEOCODE_H
#define ARCGISGEOCODE_H
#include "Bridge/Internet/httpconn.h"

class ARCGISGeocode : public HTTPConn
{
    Q_OBJECT
public:
    explicit ARCGISGeocode(const QString &databaseName, QObject *parent = nullptr);

    explicit ARCGISGeocode(const QString &databaseName,
                           const QString &serverAddress,
                           const QString &username,
                           const QString &password,
                           const QStringList &headers,
                           const int connectionFreqMS,
                           const int maxActiveConnections,
                           QObject *parent = nullptr);

    virtual ~ARCGISGeocode(){qDebug() << "~CGD";}

    void geocodeLocation(const QString &key,
                         QString address1,
                         QString address2,
                         QString city,
                         QString state,
                         QString zipCode);

    void geocodeLocation(const QString &key, const QJsonObject &locationJson);
};

#endif // ARCGISGEOCODE_H
