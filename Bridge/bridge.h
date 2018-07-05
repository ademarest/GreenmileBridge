#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include "Greenmile/gmconnection.h"
#include "MasterRoute/mrsconnection.h"
#include "AS400/as400connection.h"
#include "MasterRouteData/mrsdataconnection.h"
#include "Bridge/bridgedatabase.h"

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
    void handleMasterRouteSheetData(QJsonObject sheetData);
    void handleRouteKeysForDate(QJsonArray routeArray);
    void handleRouteComparisonInfo(QJsonArray routeArray);
    void handleLocationKeys(QJsonArray locationArray);
    void handleRouteQueryResults(QMap<QString,QVariantList> sqlResults);
    void handleAllGreenmileOrgInfoResults(QJsonArray organizationInfo);
    void handleMasterRouteDataResults(const QString &key, const QJsonObject &sheetData);

private:
    GMConnection *gmConn = new GMConnection(this);
    MRSConnection *mrsConn = new MRSConnection(this);
    AS400 *as400Conn = new AS400(this);
    MRSDataConnection *mrsDataConn = new MRSDataConnection(this);
    BridgeDatabase *bridgeDB = new BridgeDatabase(this);

    bool gmOrganizationInfoDone_    = true;
    bool gmLocationKeysDone_        = true;
    bool gmRouteComparisonInfoDone_ = true;
    bool as400RouteQueryDone_       = true;
    bool mrsRouteDataDone_          = true;
    bool mrsDataEquipmentDone_      = true;
    bool mrsDataDriverDone_         = true;

    bool bridgeRunStatus_ = false;
    void bridgeLoop();

    void as400RouteResultToCommonForm(const QMap<QString,QVariantList> &sqlResults);
    QMap<QString, QJsonObject> as400Organizations_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonObject>>> as400Route_;
    QMap<QString,QMap<QDate,QMap<QString,QMap<int,QJsonObject>>>> as400Stops_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonObject>>> as400Driver_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonObject>>> as400Equipment_;
    QMap<QString,QMap<QDate,QMap<QString,QMap<QString,QJsonObject>>>> as400Locations_;
    QMap<QString,QMap<QDate,QMap<QString,QMap<QString,QJsonObject>>>> as400LocationTimeWindowOverrides_;

    void gmOrganizationInfoToCommonForm(const QJsonArray &organizationInfo);
    QMap<QString, QJsonObject> gmOrganizations_;

    void gmRouteComparisonInfoToCommonForm(const QJsonArray &routeComparisonInfo);
    QMap<QString,QMap<QDate,QMap<QString,QJsonObject>>> gmRoute_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonArray>>> gmDriverAssignments_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonArray>>> gmEquipmentAssignments_;
    QMap<QString,QMap<QDate,QMap<QString,QMap<QString,QJsonObject>>>> gmRouteLocations_;
    QMap<QString,QMap<QDate,QMap<QString,QMap<QString,QJsonObject>>>> gmRouteLocationTimeWindowOverrides_;
    void gmLocationInfoToCommonFormat(const QJsonArray &locationInfo);
    QMap<QString,QMap<QString,QJsonObject>> gmLocations_;

    void seattleMRSDailyScheduleToCommonForm(const QJsonObject &sheetData);
    QMap<QString, QJsonObject> mrsOrganizations_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonObject>>> mrsRoute_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonArray>>> mrsDriver_;
    QMap<QString,QMap<QDate,QMap<QString,QJsonArray>>>  mrsEquipment_;

    void mrsDataDriverToCommonForm(const QJsonObject &sheetData);
    QMap<QString, QJsonObject> mrsDataDrivers_;

    void mrsDataEquipmentToCommonForm(const QJsonObject &sheetData);
    QMap<QString, QJsonObject> mrsDataEquipment_;

    void makeRoutesToUpload();

};

#endif // BRIDGE_H
