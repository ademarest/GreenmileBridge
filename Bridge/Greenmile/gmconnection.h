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
                            const int connectionFreqMS,
                            const int maxActiveConnections,
                            QObject *parent = nullptr);

    virtual ~GMConnection();

    void setServerAddress(const QString &serverAddress);
    void setUsername(const QString &username);
    void setPassword(const QString &password);

    QString getServerAddress() const;
    QString getUsername() const;

    void requestRouteKeysForDate(const QString &key, const QDate &date);
    void requestLocationKeys(const QString &key);
    void requestLocationInfo(const QString &key);
    void requestLocationOverrideTimeWindowInfo(const QString &key);
    void requestAllOrganizationInfo(const QString &key);
    void requestAllStopTypeInfo(const QString &key);
    void requestAllLocationTypeInfo(const QString &key);
    void requestRouteComparisonInfo(const QString &key, const QDate &date);
    void requestDriverInfo(const QString &key);
    void requestEquipmentInfo(const QString &key);
    void uploadARoute(const QString &key, const QJsonObject &routeJson);
    void assignDriverToRoute(const QString &key, const QJsonObject &routeDriverAssignmentJson);
    void assignEquipmentToRoute(const QString &key, const QJsonObject &routeEquipmentAssignmentJson);
    void requestAccountTypes(const QString &key);
    void requestServiceTimeTypes(const QString &key);
    void requestLocationTypes(const QString &key);
    void requestStopTypes(const QString &key);

    void deleteRoute(const QString &key, const QString &entityID);
    void deleteDriverAssignment(const QString &key, const QString &entityID);
    void deleteEquipmentAssignment(const QString &key, const QString &entityID);

    void geocodeLocation(const QString &key, const QJsonObject &locationJson);
    void uploadALocation(const QString &key, const QJsonObject &locationJson);
    void putLocation(const QString &key, const QString &entityID, const QJsonObject &locationJson);
    void patchLocation(const QString &key, const QJsonObject &locationJson);

    bool isProcessingNetworkRequests();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);
    void gmNetworkResponse(const QString &key, const QJsonValue &obj);
    void failed(const QString &key, const QString &reason);

public slots:

private slots:
    void prepConnectionQueue();
    void setReadyForNextConnection();
    void processConnectionQueue();

    void handleNetworkReply(QNetworkReply *reply);
    void startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal);
    void requestTimedOut();
private:
    void addToConnectionQueue(const QNetworkAccessManager::Operation requestType,
                              const QString &requestKey,
                              const QString &serverAddrTail,
                              const QByteArray &data = QByteArray(),
                              const QString &customOperation = QString());

    int numberOfActiveConnections_ = 0;
    bool readyForNextConnection_    = true;

    QTimer *checkQueueTimer         = new QTimer(this);
    QString dbPath_                 = qApp->applicationDirPath() + "/gmconnection.db";
    JsonSettings *settings_         = new JsonSettings(this);
    QJsonObject jsonSettings_ {{"serverAddress",        QJsonValue("https://charliesproduce.greenmile.com")},
                               {"username",             QJsonValue("username")},
                               {"password",             QJsonValue("password")},
                               {"requestTimeoutSec",    QJsonValue(40)},
                               {"connectionFreqMS",     QJsonValue(100)},
                               {"maxActiveConnections", QJsonValue(10)}};

    QNetworkAccessManager *qnam_    = new QNetworkAccessManager(this);
    QTimer *routeKeyResponseTimer_  = new QTimer(this);
    QTimer *connectionFrequencyTimer_  = new QTimer(this);

    QNetworkRequest makeGMNetworkRequest(const QString &serverAddrTail);

    //Test code loading up all network information
    QMap<QString, QNetworkAccessManager*> networkManagers_;
    QMap<QString, QNetworkReply*> networkReplies_;
    QMap<QString, QTimer*> networkTimers_;
    QMap<QString, QBuffer*> networkBuffers_;
    QSet<QString> networkRequestsInProgress_;
    QQueue<QVariantMap> connectionQueue_;
};

#endif // GMCONNECTION_H
