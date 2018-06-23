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

void GMConnection::getRouteKeysForDate(const QDate &date)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    QString serverAddrTail =    "/Route/restrictions?criteria"
                                "={\"filters\":[\"id\","
                                " \"key\"]}";

    QByteArray postData = QString("{\"attr\":\"date\", \"eq\":\"" + date.toString(Qt::ISODate) + "\"}").toLocal8Bit();
    QNetworkRequest request = makeGMNetworkRequest(serverAddrTail);

    QNetworkReply *networkReply = qnam_->post(request, postData);
    connect(networkReply, &QNetworkReply::downloadProgress, this, &GMConnection::downloadProgess);
    connect(qnam_, &QNetworkAccessManager::finished, this, &GMConnection::handleRouteKeyForDateReply);
}

void GMConnection::handleRouteKeyForDateReply(QNetworkReply *reply)
{
    QJsonArray json = QJsonDocument::fromJson(reply->readAll()).array();
    emit routeKeysForDate(json);
    disconnect(reply, &QNetworkReply::downloadProgress, this, &GMConnection::downloadProgess);
    disconnect(qnam_, &QNetworkAccessManager::finished, this, &GMConnection::handleRouteKeyForDateReply);
    reply->deleteLater();
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
