#include "mrsconnection.h"

MRSConnection::MRSConnection(QObject *parent) : QObject(parent)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
}

void MRSConnection::requestRouteKeysForDate(const QDate &date)
{
    QString key = "routeKeys";
    QUrl address = jsonSettings_["base_url"].toString() + date.toString("dddd");
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    oauth2GetRequest(key, address);
}

void MRSConnection::handleNetworkReply(QNetworkReply *reply)
{
    QString key = reply->objectName();

    if(reply->isOpen())
    {
        qDebug() << reply->readAll();
//        QJsonArray json = QJsonDocument::fromJson(reply->readAll()).array();

//        if(key == "routeKey")
//            emit routeKeysForDate(json);
//        if(key == "locationKey")
//            emit locationKeys(json);
    }

    qDebug() << "ping";
    networkRequestsInProgress_.remove(key);
    networkTimers_[key]->stop();
    networkTimers_[key]->deleteLater();
    networkReplies_[key]->deleteLater();
    networkOAuth2ReplyHandlers_[key]->deleteLater();
    networkOAuth2Flows_[key]->deleteLater();
}

void MRSConnection::startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal)
{
    //bytesTotal == 0 means the request was aborted.

    bool goodToCast = false;
    QString senderName = sender()->objectName();
    QObject *obj = sender();
    QTimer* timer;

    if(obj == networkTimers_[senderName])
        goodToCast = true;

    if(goodToCast)
        timer = qobject_cast<QTimer*>(sender());
    else
        return;

    if(bytesTotal == 0)
    {
        qDebug() << "No bytes";
        timer->stop();
        return;
    }

    timer->stop();
    timer->start(jsonSettings_["request_timeout"].toInt() * 1000);
}

void MRSConnection::requestTimedOut()
{
    QString key = sender()->objectName();
    emit statusMessage("Network request for " + key + " has timed out.");
    emit statusMessage("Aborting network call for " + key + ".");

    //Calling abort also emits finished.
    networkReplies_[key]->abort();
}

void MRSConnection::buildOAuth2(const QString &key)
{
    networkOAuth2Flows_[key] = new QOAuth2AuthorizationCodeFlow(this);
    networkOAuth2Flows_[key]->setObjectName(key);
    connect(networkOAuth2Flows_[key], &QOAuth2AuthorizationCodeFlow::granted,this, &MRSConnection::saveOAuth2TokensToDB);
    connect(networkOAuth2Flows_[key], &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,&QDesktopServices::openUrl);

    networkTimers_[key] = new QTimer(this);
    networkTimers_[key]->setObjectName(key);

    networkOAuth2Flows_[key]->setScope(jsonSettings_["api_scope"].toString());

    const auto redirectUris = jsonSettings_["redirect_uris"].toArray();
    const QUrl redirectUri(redirectUris[0].toString()); // Get the first URI
    const auto port = static_cast<quint16>(redirectUri.port()); // Get the port

    networkOAuth2Flows_[key]->setAuthorizationUrl(jsonSettings_["auth_uri"].toString());
    networkOAuth2Flows_[key]->setClientIdentifier(jsonSettings_["client_id"].toString());
    networkOAuth2Flows_[key]->setAccessTokenUrl(jsonSettings_["token_uri"].toString());
    networkOAuth2Flows_[key]->setClientIdentifierSharedKey(jsonSettings_["client_secret"].toString());

    networkOAuth2Flows_[key]->setModifyParametersFunction\
        ([&](QAbstractOAuth::Stage stage, QVariantMap *parameters)\
         {
              if(stage == QAbstractOAuth::Stage::RequestingAuthorization)
              {
                 qDebug() << "Request Access";
                 parameters->insert("approval_prompt", "force");
                 parameters->insert("access_type", "offline");
                 qDebug() << "--------------------------------";
              }
              if(stage == QAbstractOAuth::Stage::RefreshingAccessToken)
              {
                  qDebug() << "Refresh Access";
                  parameters->clear();
                  parameters->insert("client_secret", jsonSettings_["client_secret"].toString());
                  parameters->insert("refresh_token", jsonSettings_["refresh_token"].toString());
                  parameters->insert("grant_type", "refresh_token");
                  parameters->insert("client_id", jsonSettings_["client_id"].toString());
              }
          });

    networkOAuth2ReplyHandlers_[key] = new QOAuthHttpServerReplyHandler(port, this);
    networkOAuth2Flows_[key]->setReplyHandler(networkOAuth2ReplyHandlers_[key]);

    if(jsonSettings_["refresh_token"].toString().isEmpty())
    {
        waitingOnOAuth2Grant_ = true;
        networkOAuth2Flows_[key]->grant();
    }
    else if(QDateTime::fromString(jsonSettings_["expiration_at"].toString(), Qt::ISODateWithMs) < QDateTime::currentDateTime())
    {
        networkOAuth2Flows_[key]->setRefreshToken(jsonSettings_["refresh_token"].toString());
        networkOAuth2Flows_[key]->setToken(jsonSettings_["token"].toString());
        networkOAuth2Flows_[key]->refreshAccessToken();
    }
    else
    {
        networkOAuth2Flows_[key]->setRefreshToken(jsonSettings_["refresh_token"].toString());
        networkOAuth2Flows_[key]->setToken(jsonSettings_["token"].toString());
    }
}

void MRSConnection::oauth2GetRequest(const QString &key, const QUrl &address)
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);

    if(networkRequestsInProgress_.contains(key))
    {
        emit statusMessage("OAuth2 request "
                           + key
                           + " already in progress."
                             " Try again when the current request has completed.");
        return;
    }

    buildOAuth2(key);

    while(waitingOnOAuth2Grant_)
        qApp->processEvents();

    networkReplies_[key] = networkOAuth2Flows_[key]->get(address);
    networkReplies_[key]->setObjectName(key);

    connect(networkReplies_ [key],   &QNetworkReply::downloadProgress,          this, &MRSConnection::downloadProgess);
    connect(networkReplies_ [key],   &QNetworkReply::downloadProgress,          this, &MRSConnection::startNetworkTimer);
    connect(networkOAuth2Flows_[key],&QOAuth2AuthorizationCodeFlow::finished,   this, &MRSConnection::handleNetworkReply);
    connect(networkTimers_  [key],   &QTimer::timeout,                          this, &MRSConnection::requestTimedOut);

    networkTimers_[key]->stop();
    networkTimers_[key]->start(jsonSettings_["request_timeout"].toInt() * 1000);
}

void MRSConnection::saveOAuth2TokensToDB()
{
    QString key = sender()->objectName();
    jsonSettings_["token"] = networkOAuth2Flows_[key]->token();
    jsonSettings_["expiration_at"] = networkOAuth2Flows_[key]->expirationAt().toString(Qt::ISODateWithMs);

    if(!networkOAuth2Flows_[key]->refreshToken().isEmpty())
    {
        jsonSettings_["refresh_token"] = networkOAuth2Flows_[key]->refreshToken();
        networkOAuth2Flows_[key]->setRefreshToken(jsonSettings_["refresh_token"].toString());
    }

    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
    waitingOnOAuth2Grant_ = false;
}
