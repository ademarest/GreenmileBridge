#include "gmconnection.h"

GMConnection::GMConnection(QObject *parent) : HTTPConn("gmconnection.db", parent)
{
    QJsonObject headerObj = {{"headerName", "Content-Type"},
                             {"headerValue", "application/json"}};

    QJsonArray headers = jsonSettings_["headers"].toArray();
    headers.append(QJsonValue(headerObj));
    jsonSettings_["headers"] = headers;
}

GMConnection::GMConnection(const QString &databaseName,
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

GMConnection::~GMConnection()
{

}

void GMConnection::requestRouteKeysForDate(const QString &key, const QDate &date)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    QString serverAddrTail =    "/Route/restrictions?criteria"
                                "={\"filters\":[\"id\","
                                " \"key\"]}";

    QByteArray postData = QString("{\"attr\":\"date\", \"eq\":\"" + date.toString(Qt::ISODate) + "\"}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestLocationKeys(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail =    "/Location/restrictions?criteria"
                                "={\"filters\":[\"id\","
                                " \"key\", \"organization.id\", \"organization.key\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestLocationInfo(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/Location/restrictions?criteria={\"filters\":[\"*\", \"locationOverrideTimeWindows.*\", "
                             "\"locationType.id\", "
                             "\"locationType.key\", "
                             "\"organization.id\", "
                             "\"organization.key\", "
                             "\"serviceTimeType.id\", "
                             "\"serviceTimeType.key\", "
                             "\"accountType.id\" , "
                             "\"accountType.key\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestLocationOverrideTimeWindowInfo(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/LocationOverrideTimeWindow/restrictions?criteria={\"filters\":[\"*\", \"location.id\"]}";
    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::uploadALocationOverrideTimeWindow(const QString &key, const QJsonObject &data)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/LocationOverrideTimeWindow";

    QByteArray postData = QJsonDocument(data).toJson(QJsonDocument::Compact);
    qDebug() << data;
    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::updateALocationOverrideTimeWindow(const QString &key, const QJsonObject &data)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    QString serverAddrTail = "/LocationOverrideTimeWindow";

    QByteArray postData = QJsonDocument(data).toJson(QJsonDocument::Compact);
    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::deleteALocationOverrideTimeWindow(const QString &key, const QJsonObject &data)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    QString serverAddrTail("/LocationOverrideTimeWindow/" + QString::number(data.value("id").toInt()));
    QJsonValue val = data.value("id");

    addToConnectionQueue(QNetworkAccessManager::Operation::DeleteOperation, key, serverAddrTail);
}

void GMConnection::requestAllOrganizationInfo(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail =    "/Organization/restrictions?criteria"
                                "={\"filters\":[\"*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestAllStopTypeInfo(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail =    "/StopType/restrictions?criteria"
                                "={\"filters\":[\"*\",\"organization.*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestAllLocationTypeInfo(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail =    "/LocationType/restrictions?criteria"
                                "={\"filters\":[\"*\",\"organization.*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestRouteComparisonInfo(const QString &key, const QDate &date)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/Route/restrictions?criteria={\"filters\":[\"*\","
                             " \"organization.key\","
                             " \"equipmentAssignments.equipment.*\","
                             " \"driverAssignments.driver.*\","
                             " \"stops.location.*\","
                             " \"stops.location.locationOverrideTimeWindows.*\","
                             " \"stops.*\"]}";

    QByteArray postData = QString("{\"attr\":\"date\", \"eq\":\"" + date.toString(Qt::ISODate) + "\"}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestDriverInfo(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/Driver/restrictions?criteria={\"filters\":[\"*\", \"organization.*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestEquipmentInfo(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/Equipment/restrictions?criteria={\"filters\":[\"*\", \"organization.*\", \"equipmentType.*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::uploadARoute(const QString &key, const QJsonObject &routeJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/Route?resequence=false&calculatePlanning=true";

    QByteArray postData = QJsonDocument(routeJson).toJson(QJsonDocument::Compact);
    qDebug() << routeJson;
    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::assignDriverToRoute(const QString &key, const QJsonObject &routeDriverAssignmentJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/RouteDriverAssignment";

    QByteArray postData = QJsonDocument(routeDriverAssignmentJson).toJson(QJsonDocument::Compact);

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::assignEquipmentToRoute(const QString &key, const QJsonObject &routeEquipmentAssignmentJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/RouteEquipmentAssignment";

    QByteArray postData = QJsonDocument(routeEquipmentAssignmentJson).toJson(QJsonDocument::Compact);

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestAccountTypes(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/AccountType/restrictions?criteria={\"filters\":[\"*\", \"organization.id\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::uploadAccountType(const QString &key, const QJsonObject &accountTypeJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/AccountType";

    QByteArray postData = QJsonDocument(accountTypeJson).toJson(QJsonDocument::Compact);

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestServiceTimeTypes(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/ServiceTimeType/restrictions?criteria={\"filters\":[\"*\", \"organization.id\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::uploadServiceTimeType(const QString &key, const QJsonObject &serviceTimeTypeJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/ServiceTimeType";

    QByteArray postData = QJsonDocument(serviceTimeTypeJson).toJson(QJsonDocument::Compact);

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestLocationTypes(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/LocationType/restrictions?criteria={\"filters\":[\"*\", \"organization.id\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::uploadLocationType(const QString &key, const QJsonObject &locationTypeJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/LocationType";

    QByteArray postData = QJsonDocument(locationTypeJson).toJson(QJsonDocument::Compact);

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestStopTypes(const QString &key)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/StopType/restrictions?criteria={\"filters\":[\"*\", \"organization.id\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::uploadStopType(const QString &key, const QJsonObject &stopTypeJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = routeJson["key"].toString();
    QString serverAddrTail = "/StopType";

    QByteArray postData = QJsonDocument(stopTypeJson).toJson(QJsonDocument::Compact);

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::deleteRoute(const QString &key, const QString &entityID)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/Route/" + entityID;
    qDebug() << serverAddrTail;
    addToConnectionQueue(QNetworkAccessManager::Operation::DeleteOperation, key, serverAddrTail);
}

void GMConnection::deleteDriverAssignment(const QString &key, const QString &entityID)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/RouteDriverAssignment/" + entityID;
    qDebug() << serverAddrTail;
    addToConnectionQueue(QNetworkAccessManager::Operation::DeleteOperation, key, serverAddrTail);
}

void GMConnection::deleteEquipmentAssignment(const QString &key, const QString &entityID)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/RouteEquipmentAssignment/" + entityID;
    qDebug() << serverAddrTail;
    addToConnectionQueue(QNetworkAccessManager::Operation::DeleteOperation, key, serverAddrTail);
}

void GMConnection::geocodeLocation(const QString &key, const QJsonObject &locationJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/Geocode";

    QJsonObject geocodeObj;
    geocodeObj["address"] = QJsonValue(locationJson["addressLine1"].toString());
    geocodeObj["locality"] = QJsonValue(locationJson["city"].toString());
    geocodeObj["administrativeArea"] = QJsonValue(locationJson["state"].toString());
    //Commented out because a bad zip code is causes 0 results to come back.
    //Sales can't seem to get the zip correct, so best to omit.
    //geocodeObj["postalCode"] = QJsonValue(locationJson["zipCode"].toString());

    QByteArray postData = QJsonDocument(geocodeObj).toJson(QJsonDocument::Compact);
    qDebug() << key << serverAddrTail << postData;
    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::uploadALocation(const QString &key, const QJsonObject &locationJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    //QString key = "uploadLocation:" + locationJson["key"].toString();
    QString serverAddrTail = "/Location";

    QByteArray postData = QJsonDocument(locationJson).toJson(QJsonDocument::Compact);

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation, key, serverAddrTail, postData);
}

void GMConnection::putLocation(const QString &key, const QString &entityID, const QJsonObject &locationJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    QString serverAddrTail = "/Location/" + entityID;

    QByteArray postData = QJsonDocument(locationJson).toJson(QJsonDocument::Compact);
    addToConnectionQueue(QNetworkAccessManager::Operation::PutOperation, key, serverAddrTail, postData);
}

void GMConnection::patchLocation(const QString &key, const QJsonObject &locationJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    QString serverAddrTail = "/Location";

    QByteArray postData = QJsonDocument(locationJson).toJson(QJsonDocument::Compact);
    addToConnectionQueue(QNetworkAccessManager::Operation::CustomOperation, key, serverAddrTail, postData, "PATCH");
}
