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

QJsonArray GMConnection::getRouteKeysForDate(const QDate &date)
{

    qDebug() << "0";
    QString serverAddress = jsonSettings_["serverAddress"].toString()
            + "/Route/restrictions?criteria"
              "={\"filters\":[\"id\","
              " \"key\"]}";

    QByteArray postData = QString("{\"attr\":\"date\", \"eq\":\"" + date.toString(Qt::ISODate) + "\"}").toLocal8Bit();

    QNetworkRequest request;

    QString concatenated = jsonSettings_["username"].toString() + ":" + jsonSettings_["password"].toString();
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setUrl(QUrl(serverAddress));
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
    request.setRawHeader("Content-Type", "application/json");
    QNetworkReply *networkReply = qnam_->post(request, postData);

    //connect(networkReply, &QNetworkReply::downloadProgress, this, &GMConnection::downloadProgess);
    while(!networkReply->isFinished())
        qApp->processEvents();
    qDebug() << networkReply->readAll();
    delete networkReply;
}
