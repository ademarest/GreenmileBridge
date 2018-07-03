#ifndef GMCONNECTION_H
#define GMCONNECTION_H

#include "JsonSettings/jsonsettings.h"
#include <QObject>
#include <QtNetwork>

class GMConnection : public QObject
{
    Q_OBJECT
public:
    explicit GMConnection(QObject *parent = nullptr);
    explicit GMConnection(  const QString &serverAddress,
                            const QString &username,
                            const QString &password,
                            QObject *parent = nullptr);

    void setServerAddress(const QString &serverAddress);
    void setUsername(const QString &username);
    void setPassword(const QString &password);

    QString getServerAddress() const;
    QString getUsername() const;

    void requestRouteKeysForDate(const QDate &date);
    void requestLocationKeys();
    void requestAllOrganizationInfo();
    void requestRouteComparisonInfo(const QDate &date);

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);

    void routeKeysForDate(QJsonArray array);
    void routeComparisonInfo(QJsonArray array);
    void locationKeys(QJsonArray array);
    void allOrganizationInfo(QJsonArray array);

public slots:

private slots:
    void handleNetworkReply(QNetworkReply *reply);
    void startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal);
    void requestTimedOut();
    //void routeKeyResponseTimeoutCheck(QNetworkReply *reply);

private:
    QSet<QString> activeConnections_;
    QString dbPath_                 = qApp->applicationDirPath() + "/gmconnection.db";
    JsonSettings *settings_         = new JsonSettings(this);
    QJsonObject jsonSettings_ {{"serverAddress",        QJsonValue("https://charliesproduce.greenmile.com")},
                               {"username",             QJsonValue("username")},
                               {"password",             QJsonValue("password")},
                               {"requestTimeoutSec",    QJsonValue(40)}};

    QNetworkAccessManager *qnam_    = new QNetworkAccessManager(this);
    QTimer *routeKeyResponseTimer_  = new QTimer(this);

    void makeGMPostRequest(const QString &requestKey,
                           const QString &serverAddrTail,
                           const QByteArray &postData);

    QNetworkRequest makeGMNetworkRequest(const QString &serverAddrTail);

    //Test code loading up all network information
    QMap<QString, QNetworkAccessManager*> networkManagers_  = {{"routeKeys", Q_NULLPTR}};
    QMap<QString, QNetworkReply*> networkReplies_           = {{"routeKeys", Q_NULLPTR}};
    QMap<QString, QTimer*> networkTimers_                   = {{"routeKeys", Q_NULLPTR}};
    QSet<QString> networkRequestsInProgress_;
};

#endif // GMCONNECTION_H
