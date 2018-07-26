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

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);
    void finished();

public slots:
    void startBridge();

private slots:
    void handleGMResponse(const QString &key, const QJsonValue &val);
    void beginAnalysis();
    void changeToFinishedState();

private:
    BridgeDatabase *bridgeDB = new BridgeDatabase(this); 
    GMConnection *gmConn = new GMConnection(this);
    BridgeDataCollector *dataCollector = new BridgeDataCollector(this);

    QQueue<QDate> bridgeQueue;


    QJsonObject jobs_;
    QJsonObject dataBucket_;

    void applyGeocodeResponseToLocation(const QString &key, const QJsonObject &obj);
    void handleGMDriverInfo(const QJsonArray &drivers);
    void handleGMEquipmentInfo(const QJsonArray &array);
    void handleJobCompletion(const QString &key);

    //TEMP
    QTimer *bridgeTimer = new QTimer(this);

    QDate bridgeDate = QDate::currentDate();
    bool bridgeInProgress = false;

    void fixRouteAssignments(const QString &table,
                             const QString &organizationKey,
                             const QDate &bridgeDate,
                             const QString &minDelim = QString(),
                             const QString &maxDelim = QString());

    void fixDriverAssignments(const QJsonObject &reassignmentResultObj);

    void fixEquipmentAssignments(const QJsonObject &reassignmentResultObj);

    void uploadRoutes(const QString &table,
                      const QString &organizationKey,
                      const QDate &bridgeDate,
                      const QString &minDelim = QString(),
                      const QString &maxDelim = QString());

    void uploadLocations(const QString &table,
                         const QString &organizationKey,
                         const QDate &bridgeDate,
                         const QString &minDelim = QString(),
                         const QString &maxDelim = QString());
};

#endif // BRIDGE_H
