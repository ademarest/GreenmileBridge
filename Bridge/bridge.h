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

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);

public slots:
    void startBridge();
    void handleGMResponse(const QString &key, const QJsonValue &val);
    void handleRouteQueryResults(const QMap<QString, QVariantList> &sql);
    void handleMRSDailyScheduleSQL(const QMap<QString, QVariantList> &sql);
    void handleGMLocationInfo(const QJsonArray &array);
    void handleAllGreenmileOrgInfoResults(const QJsonArray &array);
    void handleRouteComparisonInfo(const QJsonArray &array);
    void routeMRSDataToFunction(const QString &key, const QJsonObject &data);

private:
    GMConnection *gmConn = new GMConnection(this);
    MRSConnection *mrsConn = new MRSConnection(this);
    AS400 *as400Conn = new AS400(this);
    MRSDataConnection *mrsDataConn = new MRSDataConnection(this);
    BridgeDatabase *bridgeDB = new BridgeDatabase(this); 

    void beginGathering();
    void beginAnalysis();

    void handleMRSDataRouteStartTimes(const QJsonObject &data);
    void handleMRSDataDrivers(const QJsonObject &data);
    void handleMRSDataPowerUnits(const QJsonObject &data);
    QMap<QString,QVariantList> googleDataToSQL(bool hasHeader, const QStringList dataOrder, const QJsonObject &data);
    QSet<QString> dataGatheringJobs_;
    QJsonObject dataBucket_;
    void applyGeocodeResponseToLocation(const QString &key, const QJsonObject &obj);
    void handleGMDriverInfo(const QJsonArray &drivers);
    void handleGMEquipmentInfo(const QJsonArray &array);


    //TEMP
    QTimer *bridgeTimer = new QTimer(this);
    QDate bridgeDate = QDate::currentDate();
    bool bridgeInProgress = false;
};

#endif // BRIDGE_H
