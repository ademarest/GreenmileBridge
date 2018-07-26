#ifndef GMCONNECTION_H
#define GMCONNECTION_H

#include "JsonSettings/jsonsettings.h"
#include <QObject>
#include <QtNetwork>
#include <unistd.h>

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

    void requestRouteKeysForDate(const QString &key, const QDate &date);
    void requestLocationKeys(const QString &key);
    void requestLocationInfo(const QString &key);
    void requestAllOrganizationInfo(const QString &key);
    void requestAllStopTypeInfo(const QString &key);
    void requestAllLocationTypeInfo(const QString &key);
    void requestRouteComparisonInfo(const QString &key, const QDate &date);
    void requestDriverInfo(const QString &key);
    void requestEquipmentInfo(const QString &key);
    void uploadARoute(const QString &key, const QJsonObject &routeJson);
    void assignDriverToRoute(const QString &key, const QJsonObject &routeDriverAssignmentJson);
    void assignEquipmentToRoute(const QString &key, const QJsonObject &routeEquipmentAssignmentJson);
    void deleteDriverAssignment(const QString &key, const int entityID);
    void deleteEquipmentAssignment(const QString &key, const int entityID);
    void geocodeLocation(const QString &key, const QJsonObject &locationJson);
    void uploadALocation(const QString &key, const QJsonObject &locationJson);

    bool isProcessingNetworkRequests();

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);

    void gmNetworkResponse(const QString &key, const QJsonValue &obj);

public slots:

private slots:
    void processConnectionQueue();
    void handleNetworkReply(QNetworkReply *reply);
    void startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal);
    void requestTimedOut();
    //void routeKeyResponseTimeoutCheck(QNetworkReply *reply);

private:
    QSet<QString> activeConnections_;

    void addToConnectionQueue(const QNetworkAccessManager::Operation requestType,
                              const QString &requestKey,
                              const QString &serverAddrTail,
                              const QByteArray &data = QByteArray());

    QTimer *checkQueueTimer         = new QTimer(this);
    QString dbPath_                 = qApp->applicationDirPath() + "/gmconnection.db";
    JsonSettings *settings_         = new JsonSettings(this);
    QJsonObject jsonSettings_ {{"serverAddress",        QJsonValue("https://charliesproduce.greenmile.com")},
                               {"username",             QJsonValue("username")},
                               {"password",             QJsonValue("password")},
                               {"requestTimeoutSec",    QJsonValue(40)}};

    QNetworkAccessManager *qnam_    = new QNetworkAccessManager(this);
    QTimer *routeKeyResponseTimer_  = new QTimer(this);

    QNetworkRequest makeGMNetworkRequest(const QString &serverAddrTail);

    //Test code loading up all network information
    QMap<QString, QNetworkAccessManager*> networkManagers_;
    QMap<QString, QNetworkReply*> networkReplies_;
    QMap<QString, QTimer*> networkTimers_;
    QSet<QString> networkRequestsInProgress_;
    QQueue<QVariantMap> connectionQueue_;
};

#endif // GMCONNECTION_H
