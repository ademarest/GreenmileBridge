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
    explicit BridgeDataCollector(QObject *parent = nullptr);
    bool hasActiveJobs();

    void addRequest(const QString &key, const QDate date);
    void removeRequest(const QString &key);

signals:
    void finished();
    void progress(const int remainingJobs, const int totalJobs);
    void statusMessage(const QString &dbg);
    void debugMessage(const QString &dbg);
    void errorMessage(const QString &dbg);

private slots:
    void handleSQLResponse(const QString &key, const QMap<QString, QVariantList> &sql);
    void handleJsonResponse(const QString &key, const QJsonValue &jVal);

public slots:

private:
    QQueue<QPair<QString, QDate>> gatheringQueue;

    QSet<QString> activeJobs_;

    void prepDatabases();
    void beginGathering(const QDate &date);
    BridgeDatabase *bridgeDB                = new BridgeDatabase(this);
    MRSConnection *mrsConn                  = new MRSConnection("mrsconnection.db", this);
    MRSConnection *dlmrsConn                = new MRSConnection("dlmrsconnection.db", this);
    GoogleSheetsConnection *routeSheetData  = new GoogleSheetsConnection("mrsdataconnection.db", this);
    GMConnection *gmConn                    = new GMConnection(this);
    AS400 *as400                            = new AS400(this);
    QTimer *queueTimer                      = new QTimer(this);

    int totalJobs_ = 0;
    QString currentKey_;

    void handleAS400RouteQuery(const QMap<QString, QVariantList> &sql);

    void handleGMLocationInfo(const QJsonArray &array);
    void handleGMDriverInfo(const QJsonArray &array);
    void handleGMEquipmentInfo(const QJsonArray &array);
    void handleGMOrganizationInfo(const QJsonArray &array);
    void handleGMRouteInfo(const QJsonArray &array);

    void handleRSAssignments(const QString &tableName, const QMap<QString, QVariantList> &sql);
    void handleRSDrivers(const QJsonObject &data);
    void handleRSPowerUnits(const QJsonObject &data);
    void handleRSRouteStartTimes(const QJsonObject &data);
    void handleRSRouteOverrides(const QJsonObject &data);
    void handleJobCompletion(const QString &key);

    void processQueue();
};

#endif // BRIDGEDATACOLLECTOR_H
