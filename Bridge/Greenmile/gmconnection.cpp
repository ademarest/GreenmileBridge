#include "gmconnection.h"

GMConnection::GMConnection(QObject *parent) : QObject(parent)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
}

GMConnection::GMConnection(const QString &serverAddress, const QString &username, const QString &password, QObject *parent) : QObject(parent)
{
    jsonSettings_["serverAddress"] = QJsonValue(serverAddress);
    jsonSettings_["username"]      = QJsonValue(username);
    jsonSettings_["password"]      = QJsonValue(password);
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
}

void GMConnection::requestRouteKeysForDate(const QDate &date)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "routeKey";
    QString serverAddrTail =    "/Route/restrictions?criteria"
                                "={\"filters\":[\"id\","
                                " \"key\"]}";

    QByteArray postData = QString("{\"attr\":\"date\", \"eq\":\"" + date.toString(Qt::ISODate) + "\"}").toLocal8Bit();

    makeGMPostRequest(key, serverAddrTail, postData);
}

void GMConnection::requestLocationKeys()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    QString key = "locationKey";
    QString serverAddrTail =    "/Location/restrictions?criteria"
                                "={\"filters\":[\"id\","
                                " \"key\"]}";

    QByteArray postData = QString("{}").toLocal8Bit();

    makeGMPostRequest(key, serverAddrTail, postData);
}

void GMConnection::makeGMPostRequest(const QString &requestKey,
                                     const QString &serverAddrTail,
                                     const QByteArray &postData)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    if(networkRequestsInProgress_.contains(requestKey))
    {
        emit statusMessage("Net request "
                           + requestKey
                           + " already in progress."
                             " Try again when the current request has completed.");
        return;
    }

    QNetworkRequest request = makeGMNetworkRequest(serverAddrTail);

    networkTimers_[requestKey] = new QTimer(this);
    networkTimers_[requestKey]->setObjectName(requestKey);

    networkManagers_[requestKey] = new QNetworkAccessManager(this);
    networkManagers_[requestKey]->setObjectName(requestKey);

    networkReplies_[requestKey] = networkManagers_[requestKey]->post(request,postData);
    networkReplies_[requestKey]->setObjectName(requestKey);

    connect(networkReplies_ [requestKey],   &QNetworkReply::downloadProgress,   this, &GMConnection::downloadProgess);
    connect(networkReplies_ [requestKey],   &QNetworkReply::downloadProgress,   this, &GMConnection::startNetworkTimer);
    connect(networkManagers_[requestKey],   &QNetworkAccessManager::finished,   this, &GMConnection::handleNetworkReply);
    connect(networkTimers_  [requestKey],   &QTimer::timeout,                   this, &GMConnection::requestTimedOut);

    networkRequestsInProgress_.insert(requestKey);

    networkTimers_[requestKey]->stop();
    networkTimers_[requestKey]->start(jsonSettings_["requestTimeoutSec"].toInt() * 1000);
}

void GMConnection::handleNetworkReply(QNetworkReply *reply)
{
    QString key = reply->objectName();

    if(reply->isOpen())
    {
        QJsonArray json = QJsonDocument::fromJson(reply->readAll()).array();

        if(json.isEmpty())
        {
            emit statusMessage("Empty result set for " + key + ". Check network connections.");
            emit statusMessage(reply->errorString());

        }
        else
        {
            if(key == "routeKey")
                emit routeKeysForDate(json);
            if(key == "locationKey")
                emit locationKeys(json);
        }
    }

    networkRequestsInProgress_.remove(key);
    networkTimers_[key]->stop();
    networkTimers_[key]->deleteLater();
    networkManagers_[key]->deleteLater();
    networkReplies_[key]->deleteLater();
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
        emit statusMessage("Network request was null or stopped for Greenmile " + senderName);
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
