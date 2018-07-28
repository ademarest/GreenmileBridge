#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include "Bridge/bridgedatabase.h"
#include "Bridge/bridgedatacollector.h"
#include "Bridge/BridgeServices/locationgeocode.h"
#include "Bridge/BridgeServices/locationupload.h"
#include "Bridge/BridgeServices/routeupload.h"
#include "Bridge/BridgeServices/routeassignmentcorrection.h"
#include "Greenmile/gmconnection.h"

class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject *parent = nullptr);
    bool hasActiveJobs();
    void addRequest(const QString &key);
    void removeRequest(const QString &key);

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);

    void finished(const QString &key);
//    void stageFinished(const QString &key);
//    void bridgeFinished();
//    void geocodingFinished();
//    void locationUploadFinished();
//    void routeUploadFinished();
//    void routeUploadAssignmentsFinished();
//    void routeAssignmentCorrectionsFinished();

public slots:
//    void startBridge();

private slots:
//    void handleGMResponse(const QString &key, const QJsonValue &val);
//    void beginAnalysis(const QDate &date);
    void handleJobCompletion(const QString &key);

    void finishedDataCollection(const QString &key);
    void finishedLocationGeocode(const QString& key, const QJsonObject &result);
    void finishedLocationUpload(const QString &key, const QJsonObject &result);
    void finishedRouteUpload(const QString &key, const QJsonObject &result);
    void finishedRouteAssignmentCorrections(const QString &key, const QJsonObject &result);

private:
    //SETTINGS SUBSECTION
    //void loadSettings();
    //        QStringList pkList {"route:key", "route:date", "organization:key"};

    QJsonObject settings_ {{"daysToUpload", QJsonValue(QJsonArray{QDate::currentDate().toString(Qt::ISODate), QDate::currentDate().addDays(1).toString(Qt::ISODate)})},
                           {"scheduleTables", QJsonValue(QJsonArray{QJsonValue(QJsonObject{{"tableName", QJsonValue("dlmrsDailyAssignments")}}),
                                                                    QJsonValue(QJsonObject{{"tableName", QJsonValue("mrsDailyAssignments")}, {"minRouteKey", "D"}, {"maxRouteKey", "U"}})})},
                           {"organization:key", QJsonValue("SEATTLE")},
                           {"schedulePrimaryKeys", QJsonValue(QJsonArray{"route:key", "route:date", "organization:key"})}};

    //END SETTINGS SUBSECTION


    QQueue<QVariantMap> requestQueue_;
    QList<QVariantMap> argList_;
    QVariantMap currentRequest_;
    QSet<QString> activeJobs_;

    //TIMER SUBSECTION
    QTimer *queueTimer = new QTimer(this);
    QTimer *bridgeTimer = new QTimer(this);
    //END TIMER SUBSECTION

    //BRIDGE MEMBER SUBSECTION
    BridgeDataCollector *dataCollector = new BridgeDataCollector(this);
    BridgeDatabase *bridgeDB_ = new BridgeDatabase(this);
    LocationGeocode *locationGeocode_ = new LocationGeocode(this);
    LocationUpload *locationUpload_ = new LocationUpload(this);
    RouteUpload *routeUpload_ = new RouteUpload(this);
    RouteAssignmentCorrection *routeAssignmentCorrection_ = new RouteAssignmentCorrection(this);
    //END BRIDGE MEMBER SUBSECTION

    void startOnTimer();
    void processQueue();
    void startDataCollection(const QString &key, const QDate &date);
    void applyScheduleHierarchy();
    void generateArgs();
};

//private:
//    QSet<QString> activeJobs_;
//    BridgeDatabase *bridgeDB = new BridgeDatabase(this);
//    GMConnection *gmConn = new GMConnection(this);
//    BridgeDataCollector *dataCollector = new BridgeDataCollector(this);

//    void handleJobCompletion(const QString &key);

//    void applyGeocodeResponseToLocation(const QString &key, const QJsonObject &obj);

//    void fixRouteAssignments(const QString &table,
//                             const QString &organizationKey,
//                             const QDate &bridgeDate,
//                             const QString &minDelim = QString(),
//                             const QString &maxDelim = QString());

//    void fixDriverAssignments(const QJsonObject &reassignmentResultObj);

//    void fixEquipmentAssignments(const QJsonObject &reassignmentResultObj);

//    void uploadRoutes(const QString &table,
//                      const QString &organizationKey,
//                      const QDate &bridgeDate,
//                      const QString &minDelim = QString(),
//                      const QString &maxDelim = QString());


//    void uploadLocations(const QString &table,
//                         const QString &organizationKey,
//                         const QDate &bridgeDate,
//                         const QString &minDelim = QString(),
    //                         const QString &maxDelim = QString());
#endif // BRIDGE_H
