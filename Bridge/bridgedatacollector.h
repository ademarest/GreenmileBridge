#ifndef BRIDGEDATACOLLECTOR_H
#define BRIDGEDATACOLLECTOR_H

#include <QtCore>
#include <QObject>
#include "Bridge/bridgedatabase.h"
#include "MasterRoute/mrsconnection.h"
#include "GoogleSheets/googlesheetsconnection.h"
#include "Greenmile/gmconnection.h"
#include "AS400/as400connection.h"

class BridgeDataCollector : public QObject
{
    Q_OBJECT
public:
    explicit BridgeDataCollector(QObject *parent = Q_NULLPTR);
    virtual ~BridgeDataCollector();
    bool hasActiveJobs();
    void addRequest(const QString &key, const QDate &date, const int monthsUntilCustDisabled, const QStringList &sourceOverrides = QStringList());
    void removeRequest(const QString &key);
    void buildTables();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);

    void failed(const QString &key, const QString &reason);
    void finished(const QString &key);
    void progress(const int remainingJobs, const int totalJobs);

private slots:
    void handleSQLResponse(const QString &key, const QMap<QString, QVariantList> &sql);
    void handleJsonResponse(const QString &key, const QJsonValue &jVal);
    void handleComponentFailure(const QString &key, const QString &reason);

public slots:

private:
    QQueue<QVariantMap> requestQueue_;
    QSet<QString> activeJobs_;

    void assembleKnownSources();
    void prepDatabases(const QStringList &sourceOverrides = QStringList());
    void beginGathering(const QVariantMap &request);

    BridgeDatabase *bridgeDB                = new BridgeDatabase(this);
    MRSConnection *mrsConn                  = new MRSConnection("mrsconnection.db", this);
    MRSConnection *dlmrsConn                = new MRSConnection("dlmrsconnection.db", this);
    GoogleSheetsConnection *routeSheetData  = new GoogleSheetsConnection("mrsdataconnection.db", this);
    GMConnection *gmConn                    = new GMConnection(this);
    AS400 *as400                            = new AS400(this);
    QTimer *queueTimer                      = new QTimer(this);

    QJsonObject knownSourcesJson_;

    QStringList knownSources_ =
    {"mrsDailyAssignments",
     "dlmrsDailyAssignments",
     "routeStartTimes",
     "drivers",
     "powerUnits",
     "routeOverrides",
     "gmOrganizations",
     "gmRoutes",
     "gmDrivers",
     "gmEquipment",
     "gmLocations",
     "gmLocationOverrideTimeWindows",
     "gmServiceTimeTypes",
     "gmAccountTypes",
     "gmStopTypes",
     "gmLocationTypes",
     "as400RouteQuery",
     "as400LocationQuery"};

    int totalJobs_ = 0;
    QString currentKey_;

    void handleAS400RouteQuery(const QMap<QString, QVariantList> &sql);
    void handleAS400LocationQuery(const QMap<QString, QVariantList> &sql);

    void handleGMLocationInfo(const QJsonArray &array);
    void handleGMLocationOverrideTimeWindowInfo(const QJsonArray &array);
    void handleGMDriverInfo(const QJsonArray &array);
    void handleGMEquipmentInfo(const QJsonArray &array);
    void handleGMOrganizationInfo(const QJsonArray &array);
    void handleGMRouteInfo(const QJsonArray &array);
    void handleGMServiceTimeTypes(const QJsonArray &array);
    void handleGMAccountTypes(const QJsonArray &array);
    void handleGMLocationTypes(const QJsonArray &array);
    void handleGMStopTypes(const QJsonArray &array);

    void handleRSAssignments(const QString &tableName, const QMap<QString, QVariantList> &sql);
    void handleRSDrivers(const QJsonObject &data);
    void handleRSPowerUnits(const QJsonObject &data);
    void handleRSRouteStartTimes(const QJsonObject &data);
    void handleRSRouteOverrides(const QJsonObject &data);
    void handleJobCompletion(const QString &key);

    void processQueue();

    QString gmDriverCreationQuery_ =    "CREATE TABLE `gmDrivers` "
                                        "(`id` INTEGER NOT NULL UNIQUE, "
                                        "`login` TEXT, "
                                        "`enabled` INTEGER, "
                                        "`organization:id` INTEGER, "
                                        "`organization:key` TEXT, "
                                        "`key` TEXT, "
                                        "`name` TEXT, "
                                        "`unitSystem` TEXT, "
                                        "`driverType` TEXT, "
                                        "PRIMARY KEY(`id`))";

    QStringList gmDriverExpectedKeys {"id",
                                      "login",
                                      "enabled",
                                      "organization:id",
                                      "organization:key",
                                      "key",
                                      "name",
                                      "unitSystem",
                                      "driverType"};

    QString gmEquipmentCreationQuery =  "CREATE TABLE `gmEquipment` "
                                        "(`id` INTEGER NOT NULL UNIQUE, "
                                        "`key` TEXT, "
                                        "`description` TEXT, "
                                        "`equipmentType:id` INTEGER, "
                                        "`equipmentType:key` TEXT, "
                                        "`organization:id` INTEGER, "
                                        "`organization:key` TEXT, "
                                        "`gpsUnitId` TEXT, "
                                        "`enabled` INTEGER, "
                                        "PRIMARY KEY(`id`))";

    QStringList gmEquipmentExpectedKeys {   "id",
                                            "key",
                                            "description",
                                            "equipmentType:id",
                                            "equipmentType:key",
                                            "organization:id",
                                            "organization:key",
                                            "gpsUnitId",
                                            "enabled"};

    QString as400RouteCreationQuery =   "CREATE TABLE `as400RouteQuery` "
                                        "(`driver:key` TEXT, "
                                        "`equipment:key` TEXT, "
                                        "`location:addressLine1` TEXT, "
                                        "`location:addressLine2` TEXT, "
                                        "`location:city` TEXT, "
                                        "`location:deliveryDays` TEXT, "
                                        "`location:description` TEXT, "
                                        "`accountType:key` TEXT, "
                                        "`serviceTimeType:key` TEXT, "
                                        "`locationType:key` TEXT, "
                                        "`stopType:key` TEXT, "
                                        "`location:key` TEXT, "
                                        "`location:state` TEXT, "
                                        "`location:zipCode` TEXT, "
                                        "`locationOverrideTimeWindows:closeTime` TEXT, "
                                        "`locationOverrideTimeWindows:openTime` TEXT, "
                                        "`locationOverrideTimeWindows:tw1Close` TEXT, "
                                        "`locationOverrideTimeWindows:tw1Open` TEXT, "
                                        "`locationOverrideTimeWindows:tw2Close` TEXT, "
                                        "`locationOverrideTimeWindows:tw2Open` TEXT, "
                                        "`order:cube` NUMERIC, "
                                        "`order:number` TEXT NOT NULL UNIQUE, "
                                        "`order:pieces` NUMERIC, "
                                        "`order:weight` NUMERIC, "
                                        "`organization:key` TEXT, "
                                        "`route:date` TEXT, "
                                        "`route:key` TEXT, "
                                        "`stop:baseLineSequenceNum` INT, "
                                        "PRIMARY KEY(`order:number`))";

    QString as400LocaitonCreationQuery =    "CREATE TABLE `as400LocationQuery` "
                                            "(`location:enabled` INTEGER, "
                                            "`organization:key` TEXT, "
                                            "`location:key` TEXT, "
                                            "`location:addressLine1` TEXT, "
                                            "`location:addressLine2` TEXT, "
                                            "`location:city` TEXT, "
                                            "`location:deliveryDays` TEXT, "
                                            "`location:description` TEXT, "
                                            "`location:state` TEXT, "
                                            "`location:zipCode` TEXT, "
                                            "`locationOverrideTimeWindows:closeTime` TEXT, "
                                            "`locationOverrideTimeWindows:openTime` TEXT, "
                                            "`locationOverrideTimeWindows:tw1Close` TEXT, "
                                            "`locationOverrideTimeWindows:tw1Open` TEXT, "
                                            "`locationOverrideTimeWindows:tw2Close` TEXT, "
                                            "`locationOverrideTimeWindows:tw2Open` TEXT, "
                                            "`accountType:key` TEXT, "
                                            "`serviceTimeType:key` TEXT, "
                                            "`locationType:key` TEXT, "
                                            "`stopType:key` TEXT, "
                                            "PRIMARY KEY(`location:key`))";

};

#endif // BRIDGEDATACOLLECTOR_H
