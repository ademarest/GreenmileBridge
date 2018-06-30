#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include "Greenmile/gmconnection.h"
#include "MasterRoute/mrsconnection.h"
#include "AS400/as400connection.h"

class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject *parent = nullptr);
    bool uploadRoutes();

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);

public slots:
    void startBridge();
    void stopBridge();
    void handleRouteKeysForDate(QJsonArray routeArray);
    void handleLocationKeys(QJsonArray locationArray);
    void handleRouteQueryResults(QMap<QString,QVariantList> sqlResults);

private:
    GMConnection *gmConn = new GMConnection(this);
    MRSConnection *mrsConn = new MRSConnection(this);
    AS400 *as400Conn = new AS400(this);

    bool bridgeRunStatus_ = false;
    void bridgeLoop();

    void as400RouteResultToGMJson(const QMap<QString,QVariantList> &sqlResults);
    QMap<QString,QMap<QDate,QMap<QString,QJsonObject>>> gmRoute_;
    QMap<QString,QMap<QDate,QMap<QString,QMap<int,QJsonObject>>>> gmStops_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonObject>>> gmDriver_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonObject>>> gmEquipment_;
    QMap<QString,QMap<QString,QJsonObject>> gmLocations_;
    QMap<QString,QMap<QString,QJsonObject>> gmLocationTimeWindowOverrides_;
};

#endif // BRIDGE_H
