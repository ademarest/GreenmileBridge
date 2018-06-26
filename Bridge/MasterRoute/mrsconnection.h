#ifndef MRSCONNECTION_H
#define MRSCONNECTION_H

#include "JsonSettings/jsonsettings.h"
#include <QDesktopServices>
#include <QtNetworkAuth>
#include <QtNetwork>
#include <QObject>

class MRSConnection : public QObject
{
    Q_OBJECT
public:
    explicit MRSConnection(QObject *parent = nullptr);
    void requestRouteKeysForDate(const QDate &date);

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);

public slots:

private slots:
    void handleNetworkReply(QNetworkReply *reply);
    void startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal);
    void requestTimedOut();
    void saveOAuth2TokensToDB();

private:
    bool waitingOnOAuth2Grant_ = false;
    JsonSettings *settings_ = new JsonSettings(this);
    QString dbPath_ = qApp->applicationDirPath() + "/mrsconnection.db";
    QJsonObject jsonSettings_   {{"client_id", QJsonValue()},
                                {"auth_uri", QJsonValue()},
                                {"token_uri", QJsonValue()},
                                {"auth_provider_x509_cert_url", QJsonValue()},
                                {"project_id", QJsonValue()},
                                {"client_secret", QJsonValue()},
                                {"api_scope", QJsonValue()},
                                {"base_url", QJsonValue()},
                                {"redirect_uris", QJsonArray()},
                                {"token", QJsonValue()},
                                {"expiration_at", QJsonValue()},
                                {"refresh_token", QJsonValue()},
                                {"request_timeout", QJsonValue(40)}};

    QMap<QString, QOAuth2AuthorizationCodeFlow*> networkOAuth2Flows_ =  {{"routeKeys", Q_NULLPTR}};
    QMap<QString, QOAuthHttpServerReplyHandler*> networkOAuth2ReplyHandlers_ = {{"routeKeys", Q_NULLPTR}};
    QMap<QString, QNetworkReply*> networkReplies_ = {{"routeKeys", Q_NULLPTR}};
    QMap<QString, QTimer*> networkTimers_= {{"routeKeys", Q_NULLPTR}};
    QSet<QString> networkRequestsInProgress_;

    inline void buildOAuth2(const QString &key);
    void oauth2GetRequest(const QString &key, const QUrl &address);
};

#endif // MRSCONNECTION_H
