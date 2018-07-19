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
    explicit MRSConnection(const QString &databaseName, QObject *parent = nullptr);
    void requestRouteKeysForDate(const QString &organizationKey, const QDate &date);
    void requestRouteKeysFromSheet(const QString &organizationKey, const QString &sheetName);

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);
    void oauth2AlreadyGranted();
    void routeSheetData(const QJsonObject &data);
    void mrsDailyScheduleSQL(const QMap<QString,QVariantList> &sql);

public slots:

private slots:
    void handleNetworkReply(QNetworkReply *reply);
    void oauth2RequestTimedOut();
    void startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal);
    void requestTimedOut();
    void sendNetworkRequest();

private:
    QMap<QString,QVariantList> mrsDailyScheduleJsonToSQL(const QJsonObject &data);

    JsonSettings *settings_ = new JsonSettings(this);
    QString dbPath_;
    QJsonObject jsonSettings_  {{"organization_key", QJsonValue("SEATTLE")},
                                {"date_format", QJsonValue("d-MMM-yyyy")},
                                {"driver_offset", QJsonValue(1)},
                                {"truck_offset", QJsonValue(2)},
                                {"trailer_offset", QJsonValue(3)},
                                {"client_id", QJsonValue()},
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
                                {"request_timeout", QJsonValue(40)},
                                {"oauth2_user_timeout", QJsonValue(180)}};

    QMap<QString, QOAuth2AuthorizationCodeFlow*> networkOAuth2Flows_ =  {{"routeKeys", Q_NULLPTR}};
    QMap<QString, QOAuthHttpServerReplyHandler*> networkOAuth2ReplyHandlers_ = {{"routeKeys", Q_NULLPTR}};
    QMap<QString, QNetworkReply*> networkReplies_ = {{"routeKeys", Q_NULLPTR}};
    QMap<QString, QTimer*> networkOAuth2Timers_ = {{"routeKeys", Q_NULLPTR}};
    QMap<QString, QVariantMap> networkRequestInfo_ = {{"routeKeys", QVariantMap()}};
    QMap<QString, QTimer*> networkTimers_= {{"routeKeys", Q_NULLPTR}};
    QSet<QString> networkRequestsInProgress_;

    inline void buildOAuth2(const QString &key);
    void startOAuth2Request(const QString &key);
    void saveOAuth2TokensToDB(const QString &key);
    void startOAuth2GrantTimer(const QString &key);
};

#endif // MRSCONNECTION_H
