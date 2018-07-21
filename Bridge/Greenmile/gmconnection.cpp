#include "gmconnection.h"

GMConnection::GMConnection(QObject *parent) : QObject(parent)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    checkQueueTimer->start(1000);
    connect(checkQueueTimer, &QTimer::timeout, this, &GMConnection::processConnectionQueue);
}

GMConnection::GMConnection(const QString &serverAddress, const QString &username, const QString &password, QObject *parent) : QObject(parent)
{
    jsonSettings_["serverAddress"] = QJsonValue(serverAddress);
    jsonSettings_["username"]      = QJsonValue(username);
    jsonSettings_["password"]      = QJsonValue(password);
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
    checkQueueTimer->start(1000);
    connect(checkQueueTimer, &QTimer::timeout, this, &GMConnection::processConnectionQueue);
}

void GMConnection::requestRouteKeysForDate(const QDate &date)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "routeKey";
    QString serverAddrTail =    "/Route/restrictions?criteria"
                                "={\"filters\":[\"id\","
                                " \"key\"]}";

    QByteArray postData = QString("{\"attr\":\"date\", \"eq\":\"" + date.toString(Qt::ISODate) + "\"}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestLocationKeys()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "locationKey";
    QString serverAddrTail =    "/Location/restrictions?criteria"
                                "={\"filters\":[\"id\","
                                " \"key\", \"organization.id\", \"organization.key\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestLocationInfo()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "locationInfo";
    QString serverAddrTail = "/Location/restrictions?criteria={\"filters\":[\"*\", \"locationOverrideTimeWindows.id\", \"locationType.id\", \"locationType.key\", \"organization.id\", \"organization.key\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestAllOrganizationInfo()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "allOrganizationInfo";
    QString serverAddrTail =    "/Organization/restrictions?criteria"
                                "={\"filters\":[\"*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestAllStopTypeInfo()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "allOrganizationInfo";
    QString serverAddrTail =    "/Organization/restrictions?criteria"
                                "={\"filters\":[\"*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestAllLocationTypeInfo()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "allOrganizationInfo";
    QString serverAddrTail =    "/Organization/restrictions?criteria"
                                "={\"filters\":[\"*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestRouteComparisonInfo(const QDate &date)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "routeComparisonInfo";
    QString serverAddrTail = "/Route/restrictions?criteria={\"filters\":[\"*\","
                             " \"organization.key\","
                             " \"equipmentAssignments.equipment.*\","
                             " \"driverAssignments.driver.*\","
                             " \"stops.location.*\","
                             " \"stops.location.locationOverrideTimeWindows.*\"]}";

    QByteArray postData = QString("{\"attr\":\"date\", \"eq\":\"" + date.toString(Qt::ISODate) + "\"}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestDriverInfo()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "gmDrivers";
    QString serverAddrTail = "/Driver/restrictions?criteria={\"filters\":[\"*\", \"organization.*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::requestEquipmentInfo()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "gmEquipment";
    QString serverAddrTail = "/Equipment/restrictions?criteria={\"filters\":[\"*\", \"organization.*\", \"equipmentType.*\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    addToConnectionQueue(QNetworkAccessManager::Operation::PostOperation,  key, serverAddrTail, postData);
}

void GMConnection::uploadARoute(const QString &key, const QJsonObject &routeJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/Route?resequence=false&calculatePlanning=true";

    QByteArray postData = QJsonDocument(routeJson).toJson(QJsonDocument::Compact);

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

void GMConnection::deleteDriverAssignment(const QString &key, const int entityID)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/RouteDriverAssignment/" + QString::number(entityID);
    qDebug() << serverAddrTail;
    addToConnectionQueue(QNetworkAccessManager::Operation::DeleteOperation, key, serverAddrTail);
}

void GMConnection::deleteEquipmentAssignment(const QString &key, const int entityID)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString serverAddrTail = "/RouteEquipmentAssignment/" + QString::number(entityID);
    qDebug() << serverAddrTail;
    addToConnectionQueue(QNetworkAccessManager::Operation::DeleteOperation, key, serverAddrTail);
}

void GMConnection::geocodeLocation(const QJsonObject &locationJson)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "geocode:"+locationJson["key"].toString();
    QString serverAddrTail = "/Geocode";

    QJsonObject geocodeObj;
    geocodeObj["address"] = QJsonValue(locationJson["addressLine1"].toString());
    geocodeObj["locality"] = QJsonValue(locationJson["city"].toString());
    geocodeObj["administrativeArea"] = QJsonValue(locationJson["state"].toString());
    geocodeObj["postalCode"] = QJsonValue(locationJson["zipCode"].toString());

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

void GMConnection::addToConnectionQueue(const QNetworkAccessManager::Operation requestType,
                                        const QString &requestKey,
                                        const QString &serverAddrTail,
                                        const QByteArray &data)
{
    QVariantMap requestInfo {{"requestType", requestType},
                             {"requestKey", requestKey},
                             {"serverAddrTail", serverAddrTail},
                             {"data", data}};

    connectionQueue_.enqueue(requestInfo);
}

void GMConnection::processConnectionQueue()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    for(int i = 0; i < connectionQueue_.size(); i++)
    {
        QVariantMap requestMap  = connectionQueue_.dequeue();
        int requestType         = requestMap["requestType"].toInt();
        QString requestKey      = requestMap["requestKey"].toString();
        QString serverAddrTail  = requestMap["serverAddrTail"].toString();
        QByteArray data         = requestMap["data"].toByteArray();

        if(networkRequestsInProgress_.contains(requestKey))
        {
            emit errorMessage("Net request " + requestKey + " already in progress. The request will retry when the current request has completed.");
            qDebug() << "Net request " << requestKey << " already in progress. The request will retry when the current request has completed.";
            connectionQueue_.enqueue(requestMap);
            continue;
        }

        QNetworkRequest request = makeGMNetworkRequest(serverAddrTail);

        networkTimers_[requestKey] = new QTimer(this);
        networkTimers_[requestKey]->setObjectName(requestKey);

        networkManagers_[requestKey] = new QNetworkAccessManager(this);
        networkManagers_[requestKey]->setObjectName(requestKey);

        switch(requestType)
        {
            case QNetworkAccessManager::Operation::HeadOperation :
                emit debugMessage("HeadOperation not implemented yet.");
                break;
            case QNetworkAccessManager::Operation::GetOperation :
                emit debugMessage("GetOperation not implemented yet.");
                break;
            case QNetworkAccessManager::Operation::PutOperation :
                emit debugMessage("PutOperation not implemented yet.");
                break;
            case QNetworkAccessManager::Operation::PostOperation :
                qDebug() << "post request";
                networkReplies_[requestKey] = networkManagers_[requestKey]->post(request,data);
                break;
            case QNetworkAccessManager::Operation::DeleteOperation :
                qDebug() << "delete request";
                networkReplies_[requestKey] = networkManagers_[requestKey]->deleteResource(request);
                break;
            case QNetworkAccessManager::Operation::CustomOperation :
                emit debugMessage("CustomOperation not implemented yet.");
                break;
            case QNetworkAccessManager::Operation::UnknownOperation :
                emit debugMessage("UnknownOperation not implemented yet.");
                break;
        }

        networkReplies_[requestKey]->setObjectName(requestKey);

        connect(networkReplies_ [requestKey],   &QNetworkReply::downloadProgress,   this, &GMConnection::downloadProgess);
        connect(networkReplies_ [requestKey],   &QNetworkReply::downloadProgress,   this, &GMConnection::startNetworkTimer);
        connect(networkManagers_[requestKey],   &QNetworkAccessManager::finished,   this, &GMConnection::handleNetworkReply);
        connect(networkTimers_  [requestKey],   &QTimer::timeout,                   this, &GMConnection::requestTimedOut);

        networkRequestsInProgress_.insert(requestKey);

        networkTimers_[requestKey]->stop();
        networkTimers_[requestKey]->start(jsonSettings_["requestTimeoutSec"].toInt() * 1000);
        //Keeps from the DDOS bear away from GM.
        usleep(250000);
    }
}

bool GMConnection::isProcessingNetworkRequests()
{
    usleep(100);
    if(networkRequestsInProgress_.isEmpty() &&  connectionQueue_.isEmpty())
        return false;
    else
        return true;
}

void GMConnection::handleNetworkReply(QNetworkReply *reply)
{
    bool replyValid = false;
    QString key = reply->objectName();
    QJsonArray json;
    QJsonObject jObj;
    QJsonDocument jDoc;
    qDebug() << "handleNetworkReply";
    if(reply->error() != QNetworkReply::NoError)
    {
        emit errorMessage(reply->errorString());
        qDebug() << reply->error();
    }
    if(reply->isOpen())
    {
        replyValid = true;
        jDoc = QJsonDocument::fromJson(reply->readAll());
    }

    networkRequestsInProgress_.remove(key);
    networkTimers_[key]->stop();
    networkTimers_[key]->deleteLater();
    networkManagers_[key]->deleteLater();
    networkReplies_[key]->deleteLater();

    if(replyValid)
    {
        if(jDoc.isArray())
        {
            qDebug() << "GM Response for " + key + " was an array.";
            json = jDoc.array();
            emit gmNetworkResponse(key, json);
        }
        if(jDoc.isObject())
        {
            qDebug() << "GM Response for " + key + " was an object.";
            jObj = jDoc.object();
            emit gmNetworkResponse(key, jObj);
        }
        if(json.isEmpty())
        {
            emit statusMessage("Empty result set for " + key + ". Check network connections.");
            emit statusMessage(reply->errorString());
        }
        if(key == "routeKey")
            emit routeKeysForDate(json);
        if(key == "locationKey")
            emit locationKeys(json);
        if(key == "allOrganizationInfo")
            emit allOrganizationInfo(json);
        if(key == "routeComparisonInfo")
            emit routeComparisonInfo(json);
        if(key == "locationInfo")
            emit gmLocationInfo(json);
    }    
}

void GMConnection::startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal)
{
    //handle compiler warning
    qint64 br = bytesReceived;
    br += br;

    QString key = sender()->objectName();
    QTimer* timer = networkTimers_[key];

    //bytesTotal == 0 means the request was aborted.
    if(bytesTotal == 0)
    {
        emit statusMessage("Greenmile network request was null or stopped for " + key);
        timer->stop();
        return;
    }

    timer->stop();
    timer->start(jsonSettings_["requestTimeoutSec"].toInt() * 1000);
}

void GMConnection::requestTimedOut()
{
    QString key = sender()->objectName();
    emit statusMessage("Network request for Greenmile " + key + " has timed out.");
    emit statusMessage("Aborting network call for Greenmile " + key + ".");

    //Calling abort also emits finished.
    networkReplies_[key]->abort();
}

QNetworkRequest GMConnection::makeGMNetworkRequest(const QString &serverAddrTail)
{
    QString serverAddress = jsonSettings_["serverAddress"].toString() + serverAddrTail;
    QString concatenated = jsonSettings_["username"].toString() + ":" + jsonSettings_["password"].toString();
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;

    QNetworkRequest request;
    request.setUrl(QUrl(serverAddress));
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
    request.setRawHeader("Content-Type", "application/json");
    return request;
}
