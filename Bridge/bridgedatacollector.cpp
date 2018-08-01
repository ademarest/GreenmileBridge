#include "bridgedatacollector.h"

BridgeDataCollector::BridgeDataCollector(QObject *parent) : QObject(parent)
{
    connect(mrsConn, &MRSConnection::mrsDailyScheduleSQL, this, &BridgeDataCollector::handleSQLResponse);
    connect(dlmrsConn, &MRSConnection::mrsDailyScheduleSQL, this, &BridgeDataCollector::handleSQLResponse);
    connect(routeSheetData, &GoogleSheetsConnection::data, this, &BridgeDataCollector::handleJsonResponse);
    connect(gmConn, &GMConnection::gmNetworkResponse, this, &BridgeDataCollector::handleJsonResponse);
    connect(as400, &AS400::sqlResults, this, &BridgeDataCollector::handleSQLResponse);

    //connect(bridgeDB, &BridgeDatabase::statusMessage, this, &BridgeDataCollector::statusMessage);
    connect(bridgeDB, &BridgeDatabase::errorMessage, this, &BridgeDataCollector::errorMessage);
    connect(bridgeDB, &BridgeDatabase::debugMessage, this, &BridgeDataCollector::debugMessage);

    connect(queueTimer, &QTimer::timeout, this, &BridgeDataCollector::processQueue);
    queueTimer->start(1000);
}

bool BridgeDataCollector::hasActiveJobs()
{
    if(activeJobs_.isEmpty())
        return false;
    else
        return true;
}

void BridgeDataCollector::addRequest(const QString &key, const QDate date)
{
    qDebug() << "START BridgeDataCollector::addRequest.";
    QPair<QString, QDate> gatheringRequest {key, date};
    gatheringQueue.enqueue(gatheringRequest);
    qDebug() << "END BridgeDataCollector::addRequest.";
}

void BridgeDataCollector::removeRequest(const QString &key)
{
    qDebug() << "START BridgeDataCollector::removeRequest.";
    for(int i = 0; i < gatheringQueue.size(); i++)
        if(gatheringQueue[i].first == key)
            gatheringQueue.removeAt(i);
    qDebug() << "START BridgeDataCollector::removeRequest.";
}

void BridgeDataCollector::processQueue()
{
    if(hasActiveJobs() || gatheringQueue.isEmpty())
        return;
    else
    {
        qDebug() << activeJobs_.size() << totalJobs_;
        QPair<QString, QDate> job = gatheringQueue.dequeue();
        currentKey_ = job.first;
        emit statusMessage("Starting data collection for " + currentKey_ + ".");
        beginGathering(job.second);
    }
}

void BridgeDataCollector::beginGathering(const QDate &date)
{
    activeJobs_.insert("mrsDailyAssignments");
    activeJobs_.insert("dlmrsDailyAssignments");
    activeJobs_.insert("routeStartTimes");
    activeJobs_.insert("drivers");
    activeJobs_.insert("powerUnits");
    activeJobs_.insert("routeOverrides");
    activeJobs_.insert("gmOrganizations");
    activeJobs_.insert("gmRoutes");
    activeJobs_.insert("gmDrivers");
    activeJobs_.insert("gmEquipment");
    activeJobs_.insert("gmLocations");
    activeJobs_.insert("as400RouteQuery");

    totalJobs_ = activeJobs_.size();
    prepDatabases();
    emit progress(activeJobs_.size(), totalJobs_);

    mrsConn->requestAssignments("mrsDailyAssignments", date);
    dlmrsConn->requestAssignments("dlmrsDailyAssignments", "Today");
    routeSheetData->requestValuesFromAGoogleSheet("routeStartTimes", "routeStartTimes");
    routeSheetData->requestValuesFromAGoogleSheet("drivers", "drivers");
    routeSheetData->requestValuesFromAGoogleSheet("powerUnits", "powerUnits");
    routeSheetData->requestValuesFromAGoogleSheet("routeOverrides", "routeOverrides");
    gmConn->requestAllOrganizationInfo("gmOrganizations");
    gmConn->requestRouteComparisonInfo("gmRoutes", date);
    gmConn->requestDriverInfo("gmDrivers");
    gmConn->requestEquipmentInfo("gmEquipment");
    gmConn->requestLocationInfo("gmLocations");
    as400->getRouteDataForGreenmile("as400RouteQuery", date, 0);
}

void BridgeDataCollector::prepDatabases()
{
    bridgeDB->truncateATable("as400RouteQuery");
    bridgeDB->truncateATable("gmOrganizations");
    bridgeDB->truncateATable("gmRoutes");
    bridgeDB->truncateATable("gmDrivers");
    bridgeDB->truncateATable("gmEquipment");
    bridgeDB->truncateATable("gmLocations");
    bridgeDB->truncateATable("routeStartTimes");
    bridgeDB->truncateATable("drivers");
    bridgeDB->truncateATable("powerUnits");
    bridgeDB->truncateATable("routeOverrides");
}

void BridgeDataCollector::handleSQLResponse(const QString &key, const QMap<QString, QVariantList> &sql)
{
    emit debugMessage(key + " moved has moved through the response handler.");

    if(key == "mrsDailyAssignments")
        handleRSAssignments(key, sql);
    if(key == "dlmrsDailyAssignments")
        handleRSAssignments(key, sql);
    if(key == "as400RouteQuery")
        handleAS400RouteQuery(sql);

    handleJobCompletion(key);
}

void BridgeDataCollector::handleJsonResponse(const QString &key, const QJsonValue &jVal)
{
    emit debugMessage(key + " moved has moved through the response handler.");

    if(key == "routeStartTimes")
        handleRSRouteStartTimes(jVal.toObject());
    if(key == "drivers")
        handleRSDrivers(jVal.toObject());
    if(key == "powerUnits")
        handleRSPowerUnits(jVal.toObject());
    if(key == "routeOverrides")
        handleRSRouteOverrides(jVal.toObject());
    if(key == "gmOrganizations")
        handleGMOrganizationInfo(jVal.toArray());
    if(key == "gmRoutes")
        handleGMRouteInfo(jVal.toArray());
    if(key == "gmDrivers")
        handleGMDriverInfo(jVal.toArray());
    if(key == "gmEquipment")
        handleGMEquipmentInfo(jVal.toArray());
    if(key == "gmLocations")
        handleGMLocationInfo(jVal.toArray());

    handleJobCompletion(key);
}

void BridgeDataCollector::handleJobCompletion(const QString &key)
{
    activeJobs_.remove(key);
    emit progress(activeJobs_.size(), totalJobs_);
    qDebug() << activeJobs_.size() << "out of " << totalJobs_ << " data collection jobs remaining.";
    qDebug() << activeJobs_;
    if(!hasActiveJobs())
    {
        emit statusMessage("Completed data collection for " + key + ".");
        emit finished(currentKey_);
        currentKey_.clear();
    }
}

void BridgeDataCollector::handleGMDriverInfo(const QJsonArray &array)
{
    emit statusMessage("GM driver info recieved.");

    QString tableName    = "gmDrivers";

    QString creationQuery = "CREATE TABLE `gmDrivers` "
                            "(`id` INTEGER NOT NULL UNIQUE, "
                            "`login` TEXT, "
                            "`enabled` TEXT, "
                            "`organization:id` INTEGER, "
                            "`organization:key` TEXT, "
                            "`key` TEXT, "
                            "`name` TEXT, "
                            "`unitSystem` TEXT, "
                            "`driverType` TEXT, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"id",
                              "login",
                              "enabled",
                              "organization:id",
                              "organization:key",
                              "key",
                              "name",
                              "unitSystem",
                              "driverType"};

    bridgeDB->addJsonArrayInfo(tableName, creationQuery, expectedKeys);
    bridgeDB->JSONArrayInsert(tableName, array);
}

void BridgeDataCollector::handleGMEquipmentInfo(const QJsonArray &array)
{
    emit statusMessage("GM equipment info recieved.");

    QString tableName    = "gmEquipment";

    QString creationQuery = "CREATE TABLE `gmEquipment` "
                            "(`id` INTEGER NOT NULL UNIQUE, "
                            "`key` TEXT, "
                            "`description` TEXT, "
                            "`equipmentType:id` INTEGER, "
                            "`equipmentType:key` TEXT, "
                            "`organization:id` INTEGER, "
                            "`organization:key` TEXT, "
                            "`gpsUnitId` TEXT, "
                            "`enabled` TEXT, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"id",
                              "key",
                              "description",
                              "equipmentType:id",
                              "equipmentType:key",
                              "organization:id",
                              "organization:key",
                              "gpsUnitId",
                              "enabled"};


    bridgeDB->addJsonArrayInfo(tableName, creationQuery, expectedKeys);
    bridgeDB->JSONArrayInsert(tableName, array);
}

void BridgeDataCollector::handleAS400RouteQuery(const QMap<QString, QVariantList> &sql)
{
    emit statusMessage("Route info retrieved from AS400.");

    QString tableName    = "as400RouteQuery";
    QString creationQuery = "CREATE TABLE `as400RouteQuery` "
                            "(`driver:key` TEXT, "
                            "`equipment:key` TEXT, "
                            "`location:addressLine1` TEXT, "
                            "`location:addressLine2` TEXT, "
                            "`location:city` TEXT, "
                            "`location:deliveryDays` TEXT, "
                            "`location:description` TEXT, "
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

    bridgeDB->addSQLInfo(tableName, creationQuery);
    bridgeDB->SQLDataInsert(tableName, sql);
}

void BridgeDataCollector::handleRSAssignments(const QString &tableName, const QMap<QString, QVariantList> &sql)
{
    emit statusMessage("MRS assignment info recieved for "+ tableName +".");

    QString creationQuery = "CREATE TABLE `" + tableName + "` "
                            "(`route:key` TEXT NOT NULL, "
                            "`route:date` TEXT NOT NULL, "
                            "`organization:key` TEXT NOT NULL, "
                            "`driver:name` TEXT, "
                            "`truck:key` TEXT, "
                            "`trailer:key` TEXT, "
                            "PRIMARY KEY(`route:key`, `route:date`, `organization:key`))";

    bridgeDB->addSQLInfo(tableName, creationQuery);
    bridgeDB->SQLDataInsert(tableName, sql);
}

void BridgeDataCollector::handleGMLocationInfo(const QJsonArray &array)
{
    emit statusMessage("GM location info recieved.");

    QString tableName     = "gmLocations";

    QString creationQuery = "CREATE TABLE `gmLocations` "
                            "(`id` INTEGER NOT NULL UNIQUE, "
                            "`key` TEXT, "
                            "`description` TEXT, "
                            "`addressLine1` TEXT, "
                            "`addressLine2` TEXT, "
                            "`city` TEXT, "
                            "`state` TEXT, "
                            "`zipCode` TEXT, "
                            "`latitude` NUMERIC, "
                            "`longitude` NUMERIC, "
                            "`geocodingQuality` TEXT, "
                            "`deliveryDays` TEXT, "
                            "`enabled` TEXT, "
                            "`hasGeofence` TEXT, "
                            "`organization:id` INTEGER, "
                            "`organization:key` TEXT, "
                            "`locationOverrideTimeWindows:0:id` INTEGER, "
                            "`locationType:id` INTEGER, "
                            "`locationType:key` TEXT, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"id",
                              "key",
                              "description",
                              "addressLine1",
                              "addressLine2",
                              "city",
                              "state",
                              "zipCode",
                              "latitude",
                              "longitude",
                              "geocodingQuality",
                              "deliveryDays",
                              "enabled",
                              "hasGeofence",
                              "organization:id",
                              "organization:key",
                              "locationOverrideTimeWindows:0:id",
                              "locationType:id",
                              "locationType:key"};

    bridgeDB->addJsonArrayInfo(tableName, creationQuery, expectedKeys);
    bridgeDB->JSONArrayInsert(tableName, array);
}

void BridgeDataCollector::handleGMOrganizationInfo(const QJsonArray &array)
{
    emit statusMessage("GM organization info recieved.");

    QString tableName     = "gmOrganizations";


    QString creationQuery = "CREATE TABLE `gmOrganizations` "
                            "(`key` TEXT, "
                            "`description` TEXT, "
                            "`id` INTEGER NOT NULL UNIQUE, "
                            "`unitSystem` TEXT, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"id",
                              "key",
                              "unitSystem",
                              "description"};

    bridgeDB->addJsonArrayInfo(tableName, creationQuery, expectedKeys);
    bridgeDB->JSONArrayInsert(tableName, array);
}

void BridgeDataCollector::handleGMRouteInfo(const QJsonArray &array)
{
    emit statusMessage("GM route info recieved.");

    QString tableName       = "gmRoutes";

    QString creationQuery = "CREATE TABLE `gmRoutes` "
                            "(`date` TEXT, "
                            "`driverAssignments` TEXT, "
                            "`driverAssignments:0:id` INTEGER, "
                            "`driverAssignments:0:driver:id` INTEGER, "
                            "`driverAssignments:0:driver:key` TEXT, "
                            "`driversName` TEXT, "
                            "`equipmentAssignments` TEXT, "
                            "`equipmentAssignments:0:id` INTEGER, "
                            "`equipmentAssignments:0:equipment:id` INTEGER, "
                            "`equipmentAssignments:0:equipment:key` TEXT, "
                            "`id` INTEGER NOT NULL UNIQUE, "
                            "`key` TEXT, "
                            "`organization` TEXT, "
                            "`organization:key` TEXT, "
                            "`organization:id` INTEGER, "
                            "`stops` TEXT, "
                            "`totalStops` NUMERIC, "
                            "`status` TEXT, "
                            "`baselineSize1` NUMERIC, "
                            "`baselineSize2` NUMERIC, "
                            "`baselineSize3` NUMERIC, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"date",
                              "driverAssignments:0:id",
                              "driverAssignments:0:driver:id",
                              "driverAssignments:0:driver:key",
                              "driverAssignments",
                              "driversName",
                              "equipmentAssignments:0:id",
                              "equipmentAssignments:0:equipment:id",
                              "equipmentAssignments:0:equipment:key",
                              "equipmentAssignments",
                              "id",
                              "key",
                              "organization:key",
                              "organization:id",
                              "organization",
                              "stops",
                              "totalStops",
                              "status",
                              "baselineSize1",
                              "baselineSize2",
                              "baselineSize3"};

    bridgeDB->addJsonArrayInfo(tableName, creationQuery, expectedKeys);
    bridgeDB->JSONArrayInsert(tableName, array);
}

void BridgeDataCollector::handleRSRouteStartTimes(const QJsonObject &data)
{
    emit statusMessage("MRS route start time info recieved.");

    QMap<QString, QVariantList> sql;
    QString tableName       = "routeStartTimes";
    QString creationQuery = "CREATE TABLE `routeStartTimes` "
                            "(`route` TEXT NOT NULL UNIQUE, "
                            "`avgStartsPrev` TEXT, "
                            "`avgStartTime` TEXT, "
                            "`mondayStartTime` TEXT, "
                            "`mondayStartsPrevDay` TEXT, "
                            "`tuesdayStartTime` TEXT, "
                            "`tuesdayStartsPrevDay` TEXT, "
                            "`wednesdayStartTime` TEXT, "
                            "`wednesdayStartsPrevDay` TEXT, "
                            "`thursdayStartTime` TEXT, "
                            "`thursdayStartsPrevDay` TEXT, "
                            "`fridayStartTime` TEXT, "
                            "`fridayStartsPrevDay` TEXT, "
                            "`saturdayStartTime` TEXT, "
                            "`saturdayStartsPrevDay` TEXT, "
                            "`sundayStartTime` TEXT, "
                            "`sundayStartsPrevDay` TEXT, "
                            "PRIMARY KEY(`route`))";

    QStringList dataOrder {"route",
                           "avgStartsPrev",
                           "avgStartTime",
                           "mondayStartTime",
                           "mondayStartsPrevDay",
                           "tuesdayStartTime",
                           "tuesdayStartsPrevDay",
                           "wednesdayStartTime",
                           "wednesdayStartsPrevDay",
                           "thursdayStartTime",
                           "thursdayStartsPrevDay",
                           "fridayStartTime",
                           "fridayStartsPrevDay",
                           "saturdayStartTime",
                           "saturdayStartsPrevDay",
                           "sundayStartTime",
                           "sundayStartsPrevDay"};

    sql = routeSheetData->googleDataToSQL(true, dataOrder, data);

    bridgeDB->addSQLInfo(tableName, creationQuery);
    bridgeDB->SQLDataInsert(tableName, sql);
}

void BridgeDataCollector::handleRSDrivers(const QJsonObject &data)
{
    emit statusMessage("MRS driver info recieved.");

    QMap<QString, QVariantList> sql;
    QString tableName       = "drivers";
    QString creationQuery = "CREATE TABLE `drivers` "
                            "(`employeeNumber` TEXT NOT NULL UNIQUE, "
                            "`employeeName` TEXT, "
                            "`eld` TEXT, "
                            "`class` TEXT, "
                            "PRIMARY KEY(`employeeNumber`))";

    QStringList dataOrder {"employeeNumber",
                           "employeeName",
                           "eld",
                           "class"};

    sql = routeSheetData->googleDataToSQL(true, dataOrder, data);
    bridgeDB->addSQLInfo(tableName, creationQuery);
    bridgeDB->SQLDataInsert(tableName, sql);
}

void BridgeDataCollector::handleRSRouteOverrides(const QJsonObject &data)
{
    emit statusMessage("MRS route overrides recieved.");

    QMap<QString, QVariantList> sql;
    QString tableName       = "routeOverrides";
    QString creationQuery = "CREATE TABLE `routeOverrides` "
                            "(`route:key` TEXT NOT NULL UNIQUE, "
                            "`organization:key` TEXT, "
                            "`monday:origin` TEXT, "
                            "`monday:destination` TEXT, "
                            "`monday:backwards` TEXT, "
                            "`tuesday:origin` TEXT, "
                            "`tuesday:destination` TEXT, "
                            "`tuesday:backwards` TEXT, "
                            "`wednesday:origin` TEXT, "
                            "`wednesday:destination` TEXT, "
                            "`wednesday:backwards` TEXT, "
                            "`thursday:origin` TEXT, "
                            "`thursday:destination` TEXT, "
                            "`thursday:backwards` TEXT, "
                            "`friday:origin` TEXT, "
                            "`friday:destination` TEXT, "
                            "`friday:backwards` TEXT, "
                            "`saturday:origin` TEXT, "
                            "`saturday:destination` TEXT, "
                            "`saturday:backwards` TEXT, "
                            "`sunday:origin` TEXT, "
                            "`sunday:destination` TEXT, "
                            "`sunday:backwards` TEXT, "
                            "PRIMARY KEY(`route:key`))";

    QStringList dataOrder {"route:key",
                           "organization:key",
                           "monday:origin",
                           "monday:destination",
                           "monday:backwards",
                           "tuesday:origin",
                           "tuesday:destination",
                           "tuesday:backwards",
                           "wednesday:origin",
                           "wednesday:destination",
                           "wednesday:backwards",
                           "thursday:origin",
                           "thursday:destination",
                           "thursday:backwards",
                           "friday:origin",
                           "friday:destination",
                           "friday:backwards",
                           "saturday:origin",
                           "saturday:destination",
                           "saturday:backwards",
                           "sunday:origin",
                           "sunday:destination",
                           "sunday:backwards"};

    sql = routeSheetData->googleDataToSQL(true, dataOrder, data);
    bridgeDB->addSQLInfo(tableName, creationQuery);
    bridgeDB->SQLDataInsert(tableName, sql);
}

void BridgeDataCollector::handleRSPowerUnits(const QJsonObject &data)
{
    emit statusMessage("MRS power unit info recieved.");


    QMap<QString, QVariantList> sql;
    QString tableName       = "powerUnits";

    QString creationQuery = "CREATE TABLE `powerUnits` "
                            "(`number` TEXT NOT NULL UNIQUE, "
                            "`type` TEXT, "
                            "`gpsUnitCode` TEXT, "
                            "`gpsType` TEXT, "
                            "`cube` TEXT, "
                            "`weight` TEXT, "
                            "`liftGate` TEXT, "
                            "PRIMARY KEY(`number`))";

    QStringList dataOrder {"number",
                           "type",
                           "gpsUnitCode",
                           "gpsType",
                           "cube",
                           "weight",
                           "liftGate"};

    sql = routeSheetData->googleDataToSQL(true, dataOrder, data);
    bridgeDB->addSQLInfo(tableName, creationQuery);
    bridgeDB->SQLDataInsert(tableName, sql);
}

