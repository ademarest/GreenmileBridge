#ifndef GOOGLESHEETSCONNECTION_H
#define GOOGLESHEETSCONNECTION_H

#include "JsonSettings/jsonsettings.h"
#include "unistd.h"
#include <QDesktopServices>
#include <QtNetworkAuth>
#include <QtNetwork>
#include <QObject>

class GoogleSheetsConnection : public QObject
{
    Q_OBJECT
public:
    explicit GoogleSheetsConnection(const QString &databaseName, QObject *parent = nullptr);
    virtual ~GoogleSheetsConnection();

    QMap<QString, QVariantList> googleDataToSQL(bool hasHeader, const QStringList dataOrder, const QJsonObject &data);
    void requestValuesFromAGoogleSheet(const QString &requestKey, const QString &sheetName);
    void changeDatabaseName(const QString &databaseName);
    QString databaseName();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);
    void oauth2AlreadyGranted();
    void data(const QString &key, const QJsonObject &data);
    void failed(const QString &key, const QString &reason);

public slots:
    void loadSettings();
    void saveSettings();

private slots:
    void handleNetworkReply(QNetworkReply *reply);
    void oauth2RequestTimedOut();
    void startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal);
    void requestTimedOut();
    void sendNetworkRequest();
    void checkListenerStatus();

private:
    void addToConnectionQueue(const int requestType,
                              const QString &requestKey,
                              const QString &address,
                              const QByteArray &data = QByteArray());

    QTimer *checkQueueTimer = new QTimer(this);
    bool manualGrantInProgress_ = false;
    JsonSettings *settings_ = new JsonSettings(this);
    QString dbPath_ = qApp->applicationDirPath() + "/mrsdataconnection.db";
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
                                {"request_timeout", QJsonValue(40)},
                                {"oauth2_user_timeout", QJsonValue(180)}};

    QMap<QString, QOAuth2AuthorizationCodeFlow*> networkOAuth2Flows_;
    QMap<QString, QOAuthHttpServerReplyHandler*> networkOAuth2ReplyHandlers_;
    QMap<QString, QNetworkReply*> networkReplies_;
    QMap<QString, QTimer*> networkOAuth2Timers_;
    QMap<QString, QVariantMap> networkRequestInfo_;
    QMap<QString, QTimer*> networkTimers_;
    QSet<QString> networkRequestsInProgress_;
    QMap<QString, QTimer*> listenerTimers_;

    void buildOAuth2(const QString &key);
    void startOAuth2Request(const QString &key);
    void saveOAuth2TokensToDB(const QString &key);
    void startOAuth2GrantTimer(const QString &key);
    void proceedToGrantStage(const QString &key);
};

#endif // GOOGLESHEETSCONNECTION_H
