#include "mrsdataconnection.h"

MRSDataConnection::MRSDataConnection(QObject *parent) : QObject(parent)
{

    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
}

void MRSDataConnection::requestValuesFromAGoogleSheet(const QString &requestKey, const QString &sheetName)
{
    QString key = requestKey;
    QUrl address = jsonSettings_["base_url"].toString() + sheetName;
    networkRequestInfo_[key]["address"] = address;
    networkRequestInfo_[key]["request_type"] = "get";

    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    startOAuth2Request(key);
}

void MRSDataConnection::handleNetworkReply(QNetworkReply *reply)
{
    QString key = reply->objectName();

    if(reply->isOpen())
    {
        QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();

        if(json.isEmpty())
        {
            emit statusMessage("Empty result set for " + key + ". Check network connections.");
            emit statusMessage(reply->errorString());
        }
        else
        {
            emit statusMessage("Google sheets retrieved, there's " + QString::number(json["values"].toArray().size()) + " rows in the sheet.");
            emit data(key, json);
        }
    }
    networkOAuth2Flows_[key]->deleteLater();
    networkOAuth2ReplyHandlers_[key]->deleteLater();
    networkOAuth2Timers_[key]->deleteLater();
    networkTimers_[key]->deleteLater();
    networkReplies_[key]->deleteLater();

    networkRequestInfo_.remove(key);
    networkRequestsInProgress_.remove(key);
    manualGrantInProgress_ = false;
}

void MRSDataConnection::oauth2RequestTimedOut()
{
    QString key = sender()->objectName();
    emit statusMessage("OAuth2 authentication request for " + key + " has timed out.");

    networkOAuth2Flows_[key]->deleteLater();
    networkOAuth2ReplyHandlers_[key]->deleteLater();
    networkOAuth2Timers_[key]->deleteLater();
    networkTimers_[key]->deleteLater();

    networkRequestInfo_.remove(key);
    networkRequestsInProgress_.remove(key);
    manualGrantInProgress_ = false;
}

void MRSDataConnection::startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal)
{
    //bytesTotal == 0 means the request was aborted.
    //Handle compiler warning;
    qint64 br = bytesReceived;
    br += br;

    QString key = sender()->objectName();
    QTimer* timer = networkTimers_[key];

    if(bytesTotal == 0)
    {
        timer->stop();
        return;
    }

    timer->stop();
    timer->start(jsonSettings_["request_timeout"].toInt() * 1000);
}

void MRSDataConnection::startOAuth2GrantTimer(const QString &key)
{
    //bytesTotal == 0 means the request was aborted.
    QTimer* timer = networkOAuth2Timers_[key];
    timer->stop();
    timer->start(jsonSettings_["oauth2_user_timeout"].toInt() * 1000);
}

void MRSDataConnection::requestTimedOut()
{
    QString key = sender()->objectName();
    emit statusMessage("Network request for Google Sheets " + key + " has timed out.");
    emit statusMessage("Aborting network call for Google Sheets " + key + ".");

    //Calling abort also emits finished.
    networkReplies_[key]->abort();
}

void MRSDataConnection::buildOAuth2(const QString &key)
{
    networkOAuth2Timers_[key] = new QTimer(this);
    networkOAuth2Timers_[key]->setObjectName(key);
    connect(networkOAuth2Timers_[key], &QTimer::timeout, this, &MRSDataConnection::oauth2RequestTimedOut);

    networkTimers_[key] = new QTimer(this);
    networkTimers_[key]->setObjectName(key);
    connect(networkTimers_[key], &QTimer::timeout, this, &MRSDataConnection::requestTimedOut);

    networkOAuth2Flows_[key] = new QOAuth2AuthorizationCodeFlow(this);
    networkOAuth2Flows_[key]->setObjectName(key);
    connect(networkOAuth2Flows_[key], &QOAuth2AuthorizationCodeFlow::granted, this, &MRSDataConnection::sendNetworkRequest);
    connect(networkOAuth2Flows_[key], &QOAuth2AuthorizationCodeFlow::finished, this, &MRSDataConnection::handleNetworkReply);
    connect(networkOAuth2Flows_[key], &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,&QDesktopServices::openUrl);
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

    while(manualGrantInProgress_)
    {
        qApp->processEvents();
    }

    if(jsonSettings_["refresh_token"].toString().isEmpty())
    {
        manualGrantInProgress_ = true;
        networkOAuth2Flows_[key]->grant();
        startOAuth2GrantTimer(key);
    }
    else if(QDateTime::fromString(jsonSettings_["expiration_at"].toString(), Qt::ISODateWithMs) < QDateTime::currentDateTime())
    {
        networkOAuth2Flows_[key]->setRefreshToken(jsonSettings_["refresh_token"].toString());
        networkOAuth2Flows_[key]->setToken(jsonSettings_["token"].toString());
        //When refreshAccessToken completed, emits granted.
        networkOAuth2Flows_[key]->refreshAccessToken();
        startOAuth2GrantTimer(key);
    }
    else
    {
        networkOAuth2Flows_[key]->setRefreshToken(jsonSettings_["refresh_token"].toString());
        networkOAuth2Flows_[key]->setToken(jsonSettings_["token"].toString());
        emit networkOAuth2Flows_[key]->granted();
    }
}

void MRSDataConnection::startOAuth2Request(const QString &key)
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
    networkRequestsInProgress_.insert(key);
    buildOAuth2(key);
}

void MRSDataConnection::saveOAuth2TokensToDB(const QString &key)
{
    jsonSettings_["token"] = networkOAuth2Flows_[key]->token();
    jsonSettings_["expiration_at"] = networkOAuth2Flows_[key]->expirationAt().toString(Qt::ISODateWithMs);

    if(!networkOAuth2Flows_[key]->refreshToken().isEmpty())
    {
        jsonSettings_["refresh_token"] = networkOAuth2Flows_[key]->refreshToken();
        networkOAuth2Flows_[key]->setRefreshToken(jsonSettings_["refresh_token"].toString());
    }
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
}

void MRSDataConnection::sendNetworkRequest()
{
    QString key         = sender()->objectName();
    QString requestType = networkRequestInfo_[key]["request_type"].toString();
    QUrl    address     = networkRequestInfo_[key]["address"].toUrl();

    if(!networkOAuth2Flows_[key]->expirationAt().isNull())
    {
        saveOAuth2TokensToDB(key);
    }

    if(requestType == "get")
    {
        networkReplies_[key] = networkOAuth2Flows_[key]->get(address);
        networkReplies_[key]->setObjectName(key);
    }

    connect(networkReplies_ [key],  &QNetworkReply::downloadProgress,  this, &MRSDataConnection::downloadProgess);
    connect(networkReplies_ [key],  &QNetworkReply::downloadProgress,  this, &MRSDataConnection::startNetworkTimer);

    networkTimers_[key]->stop();
    networkTimers_[key]->start(jsonSettings_["request_timeout"].toInt() * 1000);
}
