#include "censusgeocode.h"

CensusGeocode::CensusGeocode(const QString &databaseName, QObject *parent) : HTTPConn(databaseName, parent)
{
}

CensusGeocode::CensusGeocode(const QString &databaseName,
                             const QString &serverAddress,
                             const QString &username,
                             const QString &password,
                             const QStringList &headers,
                             const int connectionFreqMS,
                             const int maxActiveConnections,
                             QObject *parent) :
                                HTTPConn(   databaseName,
                                            serverAddress,
                                            username,
                                            password,
                                            headers,
                                            connectionFreqMS,
                                            maxActiveConnections,
                                            parent)
{

}

void CensusGeocode::geocodeLocation(const QString &key,
                                    QString address1,
                                    QString address2,
                                    QString city,
                                    QString state,
                                    QString zipCode)
{
    QString addressURL = "/geocoder/locations/onelineaddress?address=";

    address1    = address1.simplified();
    address2    = address2.simplified();
    city        = city.simplified();
    state       = state.simplified();
    zipCode     = zipCode.simplified();

    address1.replace(QString(" "), QString("+"));
    address2.replace(QString(" "), QString("+"));
    city.replace(QString(" "), QString("+"));
    state.replace(QString(" "), QString("+"));
    zipCode.replace(QString(" "), QString("+"));

    QStringList componentList = {address1, city, state};
    addressURL.append(componentList.join(", "));
    addressURL.append("&benchmark=4&format=json");
    qDebug() << "Add geocode request to queue." << addressURL;
    addToConnectionQueue(QNetworkAccessManager::Operation::GetOperation, key, addressURL);
}

