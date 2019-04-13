#include "bridgedatacollector.h"

BridgeDataCollector::BridgeDataCollector(QObject *parent) : QObject(parent)
{
    connect(routeSheetData, &GoogleSheetsConnection::data, this, &BridgeDataCollector::handleJsonResponse);
    connect(gmConn, &GMConnection::networkResponse, this, &BridgeDataCollector::handleJsonResponse);
    connect(as400, &AS400::sqlResults, this, &BridgeDataCollector::handleSQLResponse);


    connect(routeSheetData, &GoogleSheetsConnection::failed,    this, &BridgeDataCollector::handleComponentFailure);
    connect(gmConn,         &GMConnection::failed,              this, &BridgeDataCollector::handleComponentFailure);
    connect(as400,          &AS400::failed,                     this, &BridgeDataCollector::handleComponentFailure);

    //connect(bridgeDB, &BridgeDatabase::statusMessage, this, &BridgeDataCollector::statusMessage);
    connect(bridgeDB, &BridgeDatabase::errorMessage, this, &BridgeDataCollector::errorMessage);
    connect(bridgeDB, &BridgeDatabase::debugMessage, this, &BridgeDataCollector::debugMessage);

    connect(gmConn, &GMConnection::errorMessage, this, &BridgeDataCollector::errorMessage);
    connect(gmConn, &GMConnection::debugMessage, this, &BridgeDataCollector::debugMessage);

    connect(as400, &AS400::errorMessage, this, &BridgeDataCollector::errorMessage);

    connect(queueTimer, &QTimer::timeout, this, &BridgeDataCollector::processQueue);
    qDebug() << "Bridge Data Collector CTOR";
    queueTimer->start(1000);
}

BridgeDataCollector::~BridgeDataCollector()
{

}

bool BridgeDataCollector::hasActiveJobs()
{
    if(activeJobs_.isEmpty())
        return false;
    else
        return true;
}

void BridgeDataCollector::addRequest(const QString &key, const QDate &date, const int monthsUntilCustDisabled, const QStringList &sourceOverrides)
{
    qDebug() << "START BridgeDataCollector::addRequest.";
    QVariantMap request {
        {"key", key},
        {"date", date},
        {"monthsUntilCustDisabled", monthsUntilCustDisabled},
        {"sourceOverrides", sourceOverrides}
    };
    requestQueue_.enqueue(request);
    qDebug() << "END BridgeDataCollector::addRequest.";
}

void BridgeDataCollector::removeRequest(const QString &key)
{
    qDebug() << "START BridgeDataCollector::removeRequest.";
    for(int i = 0; i < requestQueue_.size(); i++)
        if(requestQueue_[i]["key"].toString() == key)
            requestQueue_.removeAt(i);
    qDebug() << "START BridgeDataCollector::removeRequest.";
}

void BridgeDataCollector::buildTables()
{
    qDebug() << "Building tables";
    knownSources_.append(scheduleSheets.keys());
    qDebug() << knownSources_;
    for(auto source: knownSources_)
    {
        handleJsonResponse(source,  QJsonValue());
        handleSQLResponse(source,   QMap<QString,QVariantList>());
    }
}

void BridgeDataCollector::processQueue()
{
    if(hasActiveJobs() || requestQueue_.isEmpty())
        return;
    else
    {
        generateScheduleSheets();
        buildTables();
        qDebug() << activeJobs_.size() << totalJobs_;
        QVariantMap request = requestQueue_.dequeue();
        currentKey_ = request["key"].toString();
        emit statusMessage("Starting data collection for " + currentKey_ + ".");
        beginGathering(request);
    }
}

void BridgeDataCollector::generateScheduleSheets()
{
    scheduleSheetSettings_ = jsonSettings_->loadSettings(QFile(scheduleSheetDbPath_), scheduleSheetSettings_);
    for(auto schedule:scheduleSheetSettings_["scheduleList"].toArray()){
        QString scheduleName = schedule.toObject()["tableName"].toString();

        QString scheduleDBName = scheduleName + ".db";
        MRSConnection *schedPtr = new MRSConnection(scheduleDBName, this);

        connect(schedPtr,   &MRSConnection::failed,                 this, &BridgeDataCollector::handleComponentFailure);
        connect(schedPtr,   &MRSConnection::mrsDailyScheduleSQL,    this, &BridgeDataCollector::handleSQLResponse);

        scheduleSheets[scheduleName] = schedPtr;
    }
}

void BridgeDataCollector::deleteScheduleSheets()
{
    qDebug() << "Deleting schedule sheets!";
    for(auto key:scheduleSheets.keys()){
        scheduleSheets[key]->deleteLater();
        knownSources_.removeAt(knownSources_.indexOf(key));
    }
    scheduleSheets.clear();
}

void BridgeDataCollector::beginGathering(const QVariantMap &request)
{
    scheduleSheetSettings_ = jsonSettings_->loadSettings(QFile(scheduleSheetDbPath_),scheduleSheetSettings_);
    generateScheduleSheets();

    QStringList sources;
    QStringList sourceOverrides = request["sourceOverrides"].toStringList();
    QDate date = request["date"].toDate();
    QString key = request["key"].toString();
    int monthsUntilCustDisabled = request["monthsUntilCustDisabled"].toInt();
    qDebug() << "monthsUntilCustDisabled" <<  monthsUntilCustDisabled << request["monthsUntilCustDisabled"].toInt();

    if(sourceOverrides.isEmpty())
        sources = knownSources_;
    else
    {
        for(auto source:sourceOverrides)
            if(knownSources_.contains(source))
                sources.append(source);

        if(sources.isEmpty())
        {
            emit errorMessage("Source overrides for " + key + "are invalid. Overrides are " + sourceOverrides.join(", ") + ".");
            emit finished(key);
            return;
        }
    }

    qDebug() << "Source order" << sources;

    prepDatabases(sources);
    for(auto source : sources)
    {
        for(auto schedKey:scheduleSheets.keys())
        {
            if(source == schedKey)
            {
                MRSConnection *schedPtr = scheduleSheets[schedKey];
                activeJobs_.insert(schedKey);
                totalJobs_ = activeJobs_.size();
                emit progress(activeJobs_.size(), totalJobs_);
                schedPtr->requestAssignments(schedKey, date);
            }
        }

        if(source == "routeStartTimes")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            routeSheetData->requestValuesFromAGoogleSheet("routeStartTimes", "routeStartTimes");
            continue;
        }

        if(source == "drivers")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            routeSheetData->requestValuesFromAGoogleSheet("drivers", "drivers");
            continue;
        }

        if(source == "powerUnits")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            routeSheetData->requestValuesFromAGoogleSheet("powerUnits", "powerUnits");
            continue;
        }

        if(source == "routeOverrides")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            routeSheetData->requestValuesFromAGoogleSheet("routeOverrides", "routeOverrides");
            continue;
        }

        if(source == "gmOrganizations")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestAllOrganizationInfo("gmOrganizations");
            continue;
        }

        if(source == "gmRoutes")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestRouteComparisonInfo("gmRoutes", date);
            continue;
        }

        if(source == "gmDrivers")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestDriverInfo("gmDrivers");
            continue;
        }

        if(source == "gmEquipment")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestEquipmentInfo("gmEquipment");
            continue;
        }

        if(source == "gmLocations")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestLocationInfo("gmLocations");
            continue;
        }

        if(source == "gmAccountTypes")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestAccountTypes("gmAccountTypes");
            continue;
        }

        if(source == "gmServiceTimeTypes")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestServiceTimeTypes("gmServiceTimeTypes");
            continue;
        }

        if(source == "gmLocationTypes")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestLocationTypes("gmLocationTypes");
            continue;
        }

        if(source == "gmStopTypes")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestStopTypes("gmStopTypes");
            continue;
        }

        if(source == "gmLocationOverrideTimeWindows")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            gmConn->requestLocationOverrideTimeWindowInfo("gmLocationOverrideTimeWindows");
            continue;
        }

        if(source == "as400RouteQuery")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            as400->getRouteDataForGreenmile("as400RouteQuery", date, 0);
            continue;
        }

        if(source == "as400LocationQuery")
        {
            activeJobs_.insert(source);
            totalJobs_ = activeJobs_.size();
            emit progress(activeJobs_.size(), totalJobs_);
            as400->getLocationDataForGreenmile("as400LocationQuery", monthsUntilCustDisabled, 0);
            continue;
        }

    }
}

void BridgeDataCollector::prepDatabases(const QStringList &sourceOverrides)
{
    if(sourceOverrides.isEmpty())
    {
        for(auto source:knownSources_)
            bridgeDB->truncateATable(source);
    }
    else
    {
        for(auto source:sourceOverrides)
            if(knownSources_.contains(source))
                bridgeDB->truncateATable(source);
    }
}

void BridgeDataCollector::handleSQLResponse(const QString &key, const QMap<QString, QVariantList> &sql)
{
    emit debugMessage(key + " moved has moved through the sql response handler.");

    for(auto schedule:scheduleSheets.keys()){
        if(schedule == key){
            handleRSAssignments(schedule, sql);
        }
    }

    if(key == "as400RouteQuery")
        handleAS400RouteQuery(sql);
    if(key == "as400LocationQuery")
        handleAS400LocationQuery(sql);

    handleJobCompletion(key);
}

void BridgeDataCollector::handleJsonResponse(const QString &key, const QJsonValue &jVal)
{
    emit debugMessage(key + " moved has moved through the json response handler.");

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
    if(key == "gmLocationOverrideTimeWindows")
        handleGMLocationOverrideTimeWindowInfo(jVal.toArray());
    if(key == "gmAccountTypes")
        handleGMAccountTypes(jVal.toArray());
    if(key == "gmServiceTimeTypes")
        handleGMServiceTimeTypes(jVal.toArray());
    if(key == "gmLocationTypes")
        handleGMLocationTypes(jVal.toArray());
    if(key == "gmStopTypes")
        handleGMStopTypes(jVal.toArray());

    handleJobCompletion(key);
}

void BridgeDataCollector::handleComponentFailure(const QString &key, const QString &reason)
{
    emit errorMessage("Failed to complete an information gathering task." + key + " Error: " + reason);
    emit failed(key, reason);
}

void BridgeDataCollector::handleJobCompletion(const QString &key)
{
    if(activeJobs_.remove(key))
    {
        if(!hasActiveJobs())
        {
            deleteScheduleSheets();
            qDebug() << "Emitting finished";
            emit statusMessage("Completed data collection for " + key + ".");
            emit finished(currentKey_);
            currentKey_.clear();
        }
    }
    emit progress(activeJobs_.size(), totalJobs_);
    qDebug() << key << "key";
    qDebug() << activeJobs_.size() << "out of " << totalJobs_ << " data collection jobs remaining.";
    qDebug() << activeJobs_;

}

void BridgeDataCollector::handleGMDriverInfo(const QJsonArray &array)
{
    emit statusMessage("GM driver info recieved.");

    QString tableName    = "gmDrivers";

    QString creationQuery = "CREATE TABLE `gmDrivers` "
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
                            "`enabled` INTEGER, "
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
                            "(`location:addressLine1` TEXT, "
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

    bridgeDB->addSQLInfo(tableName, creationQuery);
    bridgeDB->SQLDataInsert(tableName, sql);
}

void BridgeDataCollector::handleAS400LocationQuery(const QMap<QString, QVariantList> &sql)
{
    emit statusMessage("Location info retrieved from AS400.");

    QString tableName    = "as400LocationQuery";
    QString creationQuery = "CREATE TABLE `as400LocationQuery` "
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
                            "`enabled` INTEGER, "
                            "`hasGeofence` TEXT, "
                            "`organization:id` INTEGER, "
                            "`organization:key` TEXT, "
                            "`locationOverrideTimeWindows:0:id` INTEGER, "
                            "`locationType:id` INTEGER, "
                            "`locationType:key` TEXT, "
                            "`serviceTimeType:id` INTEGER, "
                            "`serviceTimeType:key` TEXT, "
                            "`accountType:id` INTEGER, "
                            "`accountType:key` TEXT, "
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
                              "locationType:key",
                              "serviceTimeType:id",
                              "serviceTimeType:key",
                              "accountType:id",
                              "accountType:key"};

    bridgeDB->addJsonArrayInfo(tableName, creationQuery, expectedKeys);
    bridgeDB->JSONArrayInsert(tableName, array);
}

void BridgeDataCollector::handleGMLocationOverrideTimeWindowInfo(const QJsonArray &array)
{
    emit statusMessage("GM location time window override info recieved.");

    QString tableName     = "gmLocationOverrideTimeWindows";

    QString creationQuery = "CREATE TABLE `gmLocationOverrideTimeWindows` "
                            "(`id` INTEGER NOT NULL UNIQUE, "
                            "`openTime` TEXT, "
                            "`closeTime` TEXT, "
                            "`tw1Open` TEXT, "
                            "`tw1Close` TEXT, "
                            "`tw2Open` TEXT, "
                            "`tw2Close` TEXT, "
                            "`monday` INTEGER, "
                            "`tuesday` INTEGER, "
                            "`wednesday` INTEGER, "
                            "`thursday` INTEGER, "
                            "`friday` INTEGER, "
                            "`saturday` INTEGER, "
                            "`sunday` INTEGER, "
                            "`location:id` INTEGER, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"id",
                              "openTime",
                              "closeTime",
                              "tw1Open",
                              "tw1Close",
                              "tw2Open",
                              "tw2Close",
                              "monday",
                              "tuesday",
                              "wednesday",
                              "thursday",
                              "friday",
                              "saturday",
                              "sunday",
                              "location:id"};

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

void BridgeDataCollector::handleGMServiceTimeTypes(const QJsonArray &array)
{
    emit statusMessage("GM service time type info recieved.");

    QString tableName       = "gmServiceTimeTypes";

    QString creationQuery = "CREATE TABLE `gmServiceTimeTypes` "
                            "(`id` INTEGER NOT NULL UNIQUE, "
                            "`organization:id` INTEGER, "
                            "`key` TEXT, "
                            "`description` TEXT, "
                            "`nonHelperFixedTimeSecs` INTEGER, "
                            "`nonHelperVarTimeSecs` INTEGER, "
                            "`helperFixedTimeSecs` INTEGER, "
                            "`helperVarTimeSecs` INTEGER, "
                            "`enabled` INTEGER, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"id",
                              "organization:id",
                              "key",
                              "description",
                              "nonHelperFixedTimeSecs",
                              "nonHelperVarTimeSecs",
                              "helperFixedTimeSecs",
                              "helperVarTimeSecs",
                              "enabled"};

    bridgeDB->addJsonArrayInfo(tableName, creationQuery, expectedKeys);
    bridgeDB->JSONArrayInsert(tableName, array);
}

void BridgeDataCollector::handleGMAccountTypes(const QJsonArray &array)
{
    emit statusMessage("GM account type info recieved.");

    QString tableName       = "gmAccountTypes";

    QString creationQuery = "CREATE TABLE `gmAccountTypes` "
                            "(`id` INTEGER NOT NULL UNIQUE, "
                            "`organization:id` INTEGER, "
                            "`key` TEXT, "
                            "`description` TEXT, "
                            "`color` TEXT, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"id",
                              "organization:id",
                              "key",
                              "description",
                              "color"};

    bridgeDB->addJsonArrayInfo(tableName, creationQuery, expectedKeys);
    bridgeDB->JSONArrayInsert(tableName, array);
}

void BridgeDataCollector::handleGMLocationTypes(const QJsonArray &array)
{
    emit statusMessage("GM location type info recieved.");

    QString tableName       = "gmLocationTypes";

    QString creationQuery = "CREATE TABLE `gmLocationTypes` "
                            "(`id` INTEGER NOT NULL UNIQUE, "
                            "`organization:id` INTEGER, "
                            "`key` TEXT, "
                            "`description` TEXT, "
                            "`showOnMobileCreate` INTEGER, "
                            "`enabled` INTEGER, "
                            "`alternativeKey` TEXT, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"id",
                              "organization:id",
                              "key",
                              "description",
                              "showOnMobileCreate",
                              "enabled",
                              "alternativeKey"};

    bridgeDB->addJsonArrayInfo(tableName, creationQuery, expectedKeys);
    bridgeDB->JSONArrayInsert(tableName, array);
}

void BridgeDataCollector::handleGMStopTypes(const QJsonArray &array)
{
    emit statusMessage("GM stop type info recieved.");

    QString tableName       = "gmStopTypes";

    QString creationQuery = "CREATE TABLE `gmStopTypes` "
                            "(`id` INTEGER NOT NULL UNIQUE, "
                            "`organization:id` INTEGER, "
                            "`key` TEXT, "
                            "`description` TEXT, "
                            "`type` TEXT, "
                            "`locationRequired` INTEGER, "
                            "PRIMARY KEY(`id`))";

    QStringList expectedKeys {"id",
                              "organization:id",
                              "key",
                              "description",
                              "type",
                              "locationRequired"};

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
