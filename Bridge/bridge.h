#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include "Bridge/bridgedatabase.h"
#include "Bridge/bridgedatacollector.h"
#include "Greenmile/gmconnection.h"

class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject *parent = nullptr);
    bool hasActiveJobs();
    void addRequest(const QString &key, const QDate date);
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

private:
    QQueue<QPair<QString, QDate>> requestQueue_;
    QSet<QString> activeJobs_;
    QString currentKey_;
    QTimer *queueTimer = new QTimer(this);
    QTimer *bridgeTimer = new QTimer(this);
    BridgeDataCollector *dataCollector = new BridgeDataCollector(this);

    void processQueue();
    void startBridge(const QString &key, const QDate &date);
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
