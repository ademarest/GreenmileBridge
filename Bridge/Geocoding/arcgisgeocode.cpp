#include "arcgisgeocode.h"

ARCGISGeocode::ARCGISGeocode(const QString &databaseName, QObject *parent) : HTTPConn(databaseName, parent)
{
}

ARCGISGeocode::ARCGISGeocode(const QString &databaseName,
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

void ARCGISGeocode::geocodeLocation(const QString &key,
                                    QString address1,
                                    QString address2,
                                    QString city,
                                    QString state,
                                    QString zipCode)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    QString addressURL = "/arcgis/rest/services/World/GeocodeServer/findAddressCandidates?f=json&singleLine=";

    address1    = address1.simplified();
    address2    = address2.simplified();
    city        = city.simplified();
    state       = state.simplified();
    zipCode     = zipCode.simplified();

    QStringList componentList = {address1, city, state};
    addressURL.append(componentList.join(","));
    addressURL.append("&outFields=Match_addr,Addr_type");
    qDebug() << "Add geocode request to queue." << addressURL;
    addToConnectionQueue(QNetworkAccessManager::Operation::GetOperation, key, addressURL);
}


void ARCGISGeocode::geocodeLocation(const QString &key, const QJsonObject &locationJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    QString addressURL = "/arcgis/rest/services/World/GeocodeServer/findAddressCandidates?f=json&singleLine=";

    QString address1 =      locationJson["addressLine1"].toString().simplified();
    QString city =          locationJson["city"].toString().simplified();
    QString state =         locationJson["state"].toString().simplified();

    QStringList componentList = {address1, city, state};
    addressURL.append(componentList.join(","));
    addressURL.append("&outFields=Match_addr,Addr_type");
    qDebug() << "Add geocode request to queue." << addressURL;
    addToConnectionQueue(QNetworkAccessManager::Operation::GetOperation, key, addressURL);
}
