#ifndef BRIDGE_H
#define BRIDGE_H

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
    void rebuild(const QString &key);

    void finishedDataCollection(const QString &key);

    void finishedLocationUpdateGeocode(const QString& key, const QJsonObject &result);
    void finishedLocationUpdate(const QString &key, const QJsonObject &result);

    void finishedLocationUploadGeocode(const QString& key, const QJsonObject &result);
    void finishedLocationUpload(const QString &key, const QJsonObject &result);

    void finishedRouteCheck(const QString &key, const QJsonObject &result);
    void finishedRouteUpload(const QString &key, const QJsonObject &result);
    void finishedRouteAssignmentCorrections(const QString &key, const QJsonObject &result);

private:
    //SETTINGS SUBSECTION
    //void loadSettings();
    //        QStringList pkList {"route:key", "route:date", "organization:key"};

    bool failState_ = false;

    QJsonObject settings_ {{"daysToUpload", QJsonValue(QJsonArray{QDate::currentDate().toString(Qt::ISODate), QDate::currentDate().addDays(1).toString(Qt::ISODate)})},
                           {"scheduleTables", QJsonValue(QJsonArray{QJsonValue(QJsonObject{{"tableName", QJsonValue("dlmrsDailyAssignments")}}),
                                                                    QJsonValue(QJsonObject{{"tableName", QJsonValue("mrsDailyAssignments")}, {"minRouteKey", "D"}, {"maxRouteKey", "U"}})})},
                           {"organization:key", QJsonValue("SEATTLE")},
                           {"monthsUntilCustDisabled", QJsonValue(3)},
                           {"schedulePrimaryKeys", QJsonValue(QJsonArray{"route:key", "route:date", "organization:key"})}};

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
    //END TIMER SUBSECTION

    //BRIDGE MEMBER SUBSECTION
    BridgeDataCollector *dataCollector = new BridgeDataCollector(this);
    BridgeDatabase *bridgeDB_ = new BridgeDatabase(this);
    LocationGeocode *locationUploadGeocode_ = new LocationGeocode(this);
    LocationUpload *locationUpload_ = new LocationUpload(this);
    LocationGeocode *locationUpdateGeocode_ = new LocationGeocode(this);
    LocationUpload *locationUpdate_ = new LocationUpload(this);
    RouteUpload *routeUpload_ = new RouteUpload(this);
    RouteAssignmentCorrection *routeAssignmentCorrection_ = new RouteAssignmentCorrection(this);
    RouteCheck *routeCheck_ = new RouteCheck(this);
    LogWriter *logger_ = new LogWriter(this);
    //END BRIDGE MEMBER SUBSECTION

    void init();
    void startOnTimer();
    void processQueue();
    void addActiveJob(const QString &key);
    void removeActiveJob(const QString &key);
    void startDataCollection(const QString &key, const QDate &date, const int monthsUntilCustDisabled);
    void applyScheduleHierarchy();
    void generateArgs();
};

#endif // BRIDGE_H
