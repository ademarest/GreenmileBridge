#ifndef BRIDGE_H
#define BRIDGE_H

#include <QtCore>
#include <algorithm>
#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include "Bridge/bridgedatabase.h"
#include "Bridge/bridgedatacollector.h"
#include "Bridge/BridgeServices/locationgeocode.h"
#include "Bridge/BridgeServices/locationupload.h"
#include "Bridge/BridgeServices/routeupload.h"
#include "Bridge/BridgeServices/routeassignmentcorrection.h"
#include "Bridge/BridgeServices/routecheck.h"
#include "Greenmile/gmconnection.h"
#include "LogWriter/logwriter.h"
#include "BridgeServices/locationoverridetimewindow.h"
#include "BridgeServices/accounttype.h"
#include "BridgeServices/servicetimetype.h"
#include "BridgeServices/locationtype.h"
#include "JsonSettings/jsonsettings.h"

class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject *parent = Q_NULLPTR);
    virtual ~Bridge();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);

    void started(const QString &key);
    void aborted(const QString &key);
    void rebuilt(const QString &key);
    void finished(const QString &key);

    void bridgeKeyChanged(const QString &key);
    void bridgeProgress(const int remainingWork, const int totalWork);
    void currentJobProgress(const int remainingWork, const int totalWork);
    void currentJobChanged(const QString &key);

public slots:
    void addRequest(const QString &key);
    bool hasActiveJobs();
    void removeRequest(const QString &key);
    void handleComponentFailure(const QString &key, const QString &reason);
    void abort();

private slots:
    void handleJobCompletion(const QString &key);
    void rebuild();

    void finishedDataCollection(const QString &key);

    void finishedAccountTypes(const QString &key, const QMap<QString, QJsonObject> &result);

    void finishedServiceTimeTypes(const QString &key, const QMap<QString, QJsonObject> &result);

    void finishedLocationTypes(const QString &key, const QMap<QString, QJsonObject> &result);

    void finishedLocationUpdateGeocode(const QString& key, const QJsonObject &result);
    void finishedLocationUpdate(const QString &key, const QJsonObject &result);

    void finishedLocationUploadGeocode(const QString& key, const QJsonObject &result);
    void finishedLocationUpload(const QString &key, const QJsonObject &result);

    void finishedLocationOverrideTimeWindows(const QString &key, const QMap<QString, QJsonObject> &results);

    void finishedRouteCheck(const QString &key, const QJsonObject &result);
    void finishedRouteUpload(const QString &key, const QJsonObject &result);
    void finishedRouteAssignmentCorrections(const QString &key, const QJsonObject &result);

    void componentsDeleted();

private:
    //SETTINGS SUBSECTION
    //void loadSettings();
    //        QStringList pkList {"route:key", "route:date", "organization:key"};

    bool abortInProcess_ = false;
    bool failState_ = false;

    JsonSettings *jsonSettings_ = new JsonSettings(this);

    QString scheduleSettingsDbPath_ = (qApp->applicationDirPath() + "/scheduleconfig.db");
    QString bridgeSettingsDbPath_   = (qApp->applicationDirPath() + "/bridgeconfig.db");

    QJsonObject settings_ {{"daysToUpload", QJsonValue(QJsonArray{QDate::currentDate().toString(Qt::ISODate), QDate::currentDate().addDays(1).toString(Qt::ISODate)})},
                           {"scheduleTables", QJsonValue(QJsonArray{QJsonValue(QJsonObject{{"tableName", QJsonValue("dlmrsDailyAssignments")}}),
                                                                    QJsonValue(QJsonObject{{"tableName", QJsonValue("mrsDailyAssignments")}})})},
                           {"organization:key", QJsonValue("SEATTLE")},
                           {"monthsUntilCustDisabled", QJsonValue(3)},
                           {"schedulePrimaryKeys", QJsonValue(QJsonArray{"route:key", "route:date", "organization:key"})}};

    QJsonObject scheduleSettings_ {{"scheduleList",QJsonArray()}};

    QJsonObject bridgeSettings_ {{"daysToUploadInt",            QJsonValue(1)},
                                {"organization:key",            QJsonValue("SEATTLE")},
                                {"monthsUntilCustDisabled",     QJsonValue(3)},
                                {"bridgeIntervalSec",           QJsonValue(600)},
                                {"actvStopTypeID",              QJsonValue("00000")},
                                {"actvLocationTypeID",          QJsonValue("00000")}};

    //END SETTINGS SUBSECTION


    QQueue<QVariantMap> requestQueue_;
    QList<QVariantMap> argList_;
    QVariantMap currentRequest_;
    QSet<QString> activeJobs_;
    int activeJobCount_ = 0;
    int totalJobCount_ = 0;

    //TIMER SUBSECTION
    QTimer *queueTimer = new QTimer(this);
    QTimer *bridgeTimer = new QTimer(this);
    QTimer *bridgeMalfunctionTimer = new QTimer(this);
    QTimer *componentDeletionTimer = new QTimer(this);
    //END TIMER SUBSECTION

    //BRIDGE MEMBER SUBSECTION
    //IF ADDING A MEMBER, REMEMBER TO UPDATE RESET AND REBUILD!!!
    QPointer<BridgeDataCollector> dataCollector = new BridgeDataCollector(this);
    QPointer<BridgeDatabase> bridgeDB_ = new BridgeDatabase(this);
    QPointer<LocationGeocode> locationUploadGeocode_ = new LocationGeocode(this);
    QPointer<LocationUpload> locationUpload_ = new LocationUpload(this);
    QPointer<LocationGeocode> locationUpdateGeocode_ = new LocationGeocode(this);
    QPointer<LocationUpload> locationUpdate_ = new LocationUpload(this);
    QPointer<RouteUpload> routeUpload_ = new RouteUpload(this);
    QPointer<RouteAssignmentCorrection> routeAssignmentCorrection_ = new RouteAssignmentCorrection(this);
    QPointer<RouteCheck> routeCheck_ = new RouteCheck(this);
    QPointer<LogWriter> logger_ = new LogWriter(this);
    QPointer<LocationOverrideTimeWindow> lotw_ = new LocationOverrideTimeWindow(this);
    QPointer<AccountType> accountType_ = new AccountType(this);
    QPointer<ServiceTimeType> serviceTimeType_ = new ServiceTimeType(this);
    QPointer<LocationType> locationType_ = new LocationType(this);
    //END BRIDGE MEMBER SUBSECTION

    void init();
    QJsonArray generateUploadDays(int daysToUpload);
    void loadSettings();
    void startOnTimer();
    void processQueue();
    void addActiveJob(const QString &key);
    void removeActiveJob(const QString &key);
    void startDataCollection(const QString &key, const QDate &date, const int monthsUntilCustDisabled);
    void applyScheduleHierarchy();
    void generateArgs();
};

#endif // BRIDGE_H
