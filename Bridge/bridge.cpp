#include "bridge.h"

Bridge::Bridge(QObject *parent) : QObject(parent)
{
    connect(bridgeDB, &BridgeDatabase::debugMessage, this, &Bridge::statusMessage);
    connect(bridgeDB, &BridgeDatabase::errorMessage, this, &Bridge::statusMessage);
    connect(bridgeDB, &BridgeDatabase::statusMessage, this, &Bridge::statusMessage);

    connect(as400Conn, &AS400::greenmileRouteInfoResults, this, &Bridge::handleRouteQueryResults);
    connect(as400Conn, &AS400::debugMessage, this, &Bridge::statusMessage);

    connect(mrsConn, &MRSConnection::mrsDailyScheduleSQL, this, &Bridge::handleMRSDailyScheduleSQL);

    connect(mrsDataConn, &MRSDataConnection::data, this, &Bridge::routeMRSDataToFunction);

    connect(gmConn, &GMConnection::allOrganizationInfo, this, &Bridge::handleAllGreenmileOrgInfoResults);
    connect(gmConn, &GMConnection::routeComparisonInfo, this, &Bridge::handleRouteComparisonInfo);
    connect(gmConn, &GMConnection::gmLocationInfo, this, &Bridge::handleGMLocationInfo);
    connect(gmConn, &GMConnection::gmNetworkResponse, this, &Bridge::handleGMResponse);
}


void Bridge::startBridge()
{
    dataGatheringJobs_.insert("mrsDailyAssignments");
    mrsConn->requestRouteKeysForDate("SEATTLE", QDate::currentDate());


    dataGatheringJobs_.insert("routeStartTimes");
    mrsDataConn->requestValuesFromAGoogleSheet("routeStartTimes", "routeStartTimes");


    dataGatheringJobs_.insert("drivers");
    mrsDataConn->requestValuesFromAGoogleSheet("drivers", "drivers");


    dataGatheringJobs_.insert("powerUnits");
    mrsDataConn->requestValuesFromAGoogleSheet("powerUnits", "powerUnits");


    dataGatheringJobs_.insert("as400RouteQuery");
    as400Conn->getRouteDataForGreenmile(QDate::currentDate(), 10000);


    dataGatheringJobs_.insert("gmOrganizations");
    gmConn->requestAllOrganizationInfo();


    dataGatheringJobs_.insert("gmRoutes");
    gmConn->requestRouteComparisonInfo(QDate::currentDate());


    dataGatheringJobs_.insert("gmDrivers");
    gmConn->requestDriverInfo();

    dataGatheringJobs_.insert("gmEquipment");
    gmConn->requestEquipmentInfo();

    dataGatheringJobs_.insert("gmLocations");
    gmConn->requestLocationInfo();
}

void Bridge::beginAnalysis()
{
    qDebug() << "BA";
    if(!dataGatheringJobs_.isEmpty())
        return;

    qDebug() << "AA";
    QJsonObject locationsToUpload = bridgeDB->getLocationsToUpload("SEATTLE", QDate::currentDate(), "D", "U");
    qDebug() << locationsToUpload;
    for(auto key:locationsToUpload.keys())
    {
        dataBucket_["geocode:" + key] = locationsToUpload[key].toObject();

        qDebug() << "before process";

        while(gmConn->isProcessingNetworkRequests())
            qApp->processEvents();

        qDebug() << "after process";

        gmConn->geocodeLocation(dataBucket_["geocode:" + key].toObject());
    }

    while(gmConn->isProcessingNetworkRequests())
        qApp->processEvents();

    qDebug() << "done geocoding!";
    qDebug() << "testing upload route...";
    QJsonObject routeUploadObj = bridgeDB->getRoutesToUpload("SEATTLE", QDate::currentDate(), "D", "U");
    for(auto key:routeUploadObj.keys())
    {
        dataBucket_[key] = routeUploadObj[key].toObject();

        qDebug() << "before route process";

        while(gmConn->isProcessingNetworkRequests())
            qApp->processEvents();

        qDebug() << "after route process";
        if(key.split(":").first() == "routeUpload")
        {
            gmConn->uploadARoute(key, dataBucket_[key].toObject());
        }

        //gmConn->geocodeLocation(dataBucket_["geocode:" + key].toObject());
    }

    while(gmConn->isProcessingNetworkRequests())
        qApp->processEvents();

    qDebug() << "am done, phew.";
}

void Bridge::handleGMResponse(const QString &key, const QJsonValue &val)
{
    qDebug() << "key just moved through handleGMResponse" << key;
    if(key.split(":").first() == "geocode")
    {
        applyGeocodeResponseToLocation(key, val.toObject());
    }
    if(key.split(":").first() == "routeUpload")
    {
        QJsonObject response = val.toObject();
        QJsonObject routeDriverAssignmentObj;
        QJsonObject routeEquipmentAssignmentObj;

        qDebug() << key << "uploaded!";
        QStringList keyList  = key.split(":");
        keyList[0] = "driverAssignment";
        QString driverAssignmentKey = keyList.join(":");


        routeDriverAssignmentObj["route"] = QJsonObject{{"id", response["id"]}};
        routeDriverAssignmentObj["driver"] = dataBucket_[driverAssignmentKey];
        gmConn->assignDriverToRoute(driverAssignmentKey, routeDriverAssignmentObj);
        //---------------------------------------------------------------------
        //Equipment time
        //---------------------------------------------------------------------
        keyList[0] = "equipmentAssignment";
        QString equipmentAssignmentKey = keyList.join(":");

        routeEquipmentAssignmentObj["route"] = QJsonObject{{"id", response["id"]}};
        routeEquipmentAssignmentObj["equipment"] = dataBucket_[equipmentAssignmentKey];
        routeEquipmentAssignmentObj["principal"] = QJsonValue(true);

        gmConn->assignEquipmentToRoute(equipmentAssignmentKey, routeEquipmentAssignmentObj);
    }
    if(key == "gmDrivers")
    {
        handleGMDriverInfo(val.toArray());
    }
    if(key == "gmEquipment")
    {
        handleGMEquipmentInfo(val.toArray());
    }
}

void Bridge::applyGeocodeResponseToLocation(const QString &key, const QJsonObject &obj)
{
    QJsonObject dbObj = dataBucket_[key].toObject();

    if(obj["status"].toString() == "OK")
    {
        dbObj["geocodingQuality"] = QJsonValue("AUTO");
        dbObj["latitude"] = obj["results"].toArray().first()["geometry"]["location"]["lat"];
        dbObj["longitude"] = obj["results"].toArray().first()["geometry"]["location"]["lng"];
    }
    else
        dbObj["geocodingQuality"] = QJsonValue("UNSUCCESSFUL");

    dataBucket_[key] = dbObj;
    qDebug() << dataBucket_[key];
}


void Bridge::handleGMDriverInfo(const QJsonArray &drivers)
{
    emit statusMessage("GM driver response recieved.");

    QString gmDriverTableName    = "gmDrivers";

    QString gmDriverCreationQuery = "CREATE TABLE `gmDrivers` "
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

    QStringList gmDriverExpectedKeys {"id",
                                      "login",
                                      "enabled",
                                      "organization:id",
                                      "organization:key",
                                      "key",
                                      "name",
                                      "unitSystem",
                                      "driverType"};

    bridgeDB->addJsonArrayInfo(gmDriverTableName, gmDriverCreationQuery, gmDriverExpectedKeys);
    bridgeDB->JSONArrayInsert(gmDriverTableName, drivers);

    dataGatheringJobs_.remove(gmDriverTableName);
    beginAnalysis();
}

void Bridge::handleGMEquipmentInfo(const QJsonArray &array)
{
    emit statusMessage("GM equipment response recieved.");

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

    dataGatheringJobs_.remove(tableName);
    beginAnalysis();
}

void Bridge::handleRouteQueryResults(const QMap<QString, QVariantList> &sql)
{
    emit statusMessage("Route info retrieved from AS400. There's " + QString::number(sql["route:key"].size()) + " stops for all Charlie's divisions" + QStringList(sql.keys()).join(", "));

    QString as400RouteQueryTableName    = "as400RouteQuery";
    QString as400RouteQueryCreationQuery = "CREATE TABLE `as400RouteQuery` "
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

    bridgeDB->addSQLInfo(as400RouteQueryTableName, as400RouteQueryCreationQuery);
    bridgeDB->SQLDataInsert("as400RouteQuery", sql);
    dataGatheringJobs_.remove("as400RouteQuery");
    beginAnalysis();
}

void Bridge::handleMRSDailyScheduleSQL(const QMap<QString, QVariantList> &sql)
{
    QString mrsDailyAssignmentTableName = "mrsDailyAssignments";
    QString mrsDailyAssignmentCreationQuery = "CREATE TABLE `mrsDailyAssignments` "
                                              "(`route:key` TEXT NOT NULL, "
                                              "`route:date` TEXT NOT NULL, "
                                              "`organization:key` TEXT NOT NULL, "
                                              "`driver:name` TEXT, "
                                              "`truck:key` TEXT, "
                                              "`trailer:key` TEXT, "
                                              "PRIMARY KEY(`route:key`, `route:date`, `organization:key`))";

    bridgeDB->addSQLInfo(mrsDailyAssignmentTableName, mrsDailyAssignmentCreationQuery);
    bridgeDB->SQLDataInsert("mrsDailyAssignments", sql);

    dataGatheringJobs_.remove("mrsDailyAssignments");
    beginAnalysis();
}

void Bridge::handleGMLocationInfo(const QJsonArray &array)
{
    emit statusMessage("Locations info retrieved from Greenmile. There are "
                       + QString::number(array.size())
                       + " locations.");

    QString gmLocationInfoTableName     = "gmLocations";

    QString gmLocationInfoCreationQuery = "CREATE TABLE `gmLocations` "
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

    QStringList gmLocationInfoExpectedKeys {"id",
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

    bridgeDB->addJsonArrayInfo(gmLocationInfoTableName, gmLocationInfoCreationQuery, gmLocationInfoExpectedKeys);
    bridgeDB->JSONArrayInsert("gmLocations", array);

    dataGatheringJobs_.remove("gmLocations");
    beginAnalysis();
}

void Bridge::handleAllGreenmileOrgInfoResults(const QJsonArray &array)
{
    emit statusMessage("Organization info retrieved from Greenmile. There's "
                       + QString::number(array.size())
                       + " organizations for all Charlie's divisions.");

    QString gmOrganizationTableName     = "gmOrganizations";


    QString gmOrganizationCreationQuery = "CREATE TABLE `gmOrganizations` "
                                          "(`key` TEXT, "
                                          "`description` TEXT, "
                                          "`id` INTEGER NOT NULL UNIQUE, "
                                          "`unitSystem` TEXT, "
                                          "PRIMARY KEY(`id`))";

    QStringList gmOrganizationExpectedKeys {"id",
                                            "key",
                                            "unitSystem",
                                            "description"};

    bridgeDB->addJsonArrayInfo(gmOrganizationTableName, gmOrganizationCreationQuery, gmOrganizationExpectedKeys);
    bridgeDB->JSONArrayInsert("gmOrganizations", array);

    dataGatheringJobs_.remove("gmOrganizations");
    beginAnalysis();
}

void Bridge::handleRouteComparisonInfo(const QJsonArray &array)
{
    emit statusMessage("Today's route comarison result recieved from Greenmile. There are "
                       + QString::number(array.size())
                       + " routes uploaded for "
                       + QDate::currentDate().toString(Qt::ISODate)
                       + ".");

    QString gmRouteQueryTableName       = "gmRoutes";

    QString gmRouteQueryCreationQuery = "CREATE TABLE `gmRoutes` "
                                        "(`date` TEXT, "
                                        "`driverAssignments` TEXT, "
                                        "`driverAssignments:0:driver:key` TEXT, "
                                        "`driversName` TEXT, "
                                        "`equipmentAssignments` TEXT, "
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

    QStringList gmRouteQueryExpectedKeys {"date",
                                          "driverAssignments:0:driver:key",
                                          "driverAssignments",
                                          "driversName",
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

    bridgeDB->addJsonArrayInfo(gmRouteQueryTableName, gmRouteQueryCreationQuery, gmRouteQueryExpectedKeys);
    bridgeDB->JSONArrayInsert("gmRoutes", array);

    dataGatheringJobs_.remove("gmRoutes");
    beginAnalysis();
}

void Bridge::routeMRSDataToFunction(const QString &key, const QJsonObject &data)
{
    emit statusMessage("MRS Data for " + key + " retrieved.");
    if(key == "routeStartTimes")
        handleMRSDataRouteStartTimes(data);
    if(key == "drivers")
        handleMRSDataDrivers(data);
    if(key == "powerUnits")
        handleMRSDataPowerUnits(data);
}


void Bridge::handleMRSDataRouteStartTimes(const QJsonObject &data)
{
    QMap<QString, QVariantList> sql;
    QString sheetName       = "routeStartTimes";
    //ROUTE	AVG STARTS PREV	AVG START TIME	MON	STARTS PREV DAY MON	TUE	STARTS PREV DAY TUE	WED	STARTS PREV DAY WED	THU	STARTS PREV DAY THU	FRI	STARTS PREV DAY FRI	SAT	STARTS PREV DAY SAT	SUN	STARTS PREV DAY SUN
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

    sql = googleDataToSQL(true, dataOrder, data);
    bridgeDB->addSQLInfo(sheetName, creationQuery);
    bridgeDB->SQLDataInsert(sheetName, sql);

    dataGatheringJobs_.remove("routeStartTimes");
    beginAnalysis();
}

void Bridge::handleMRSDataDrivers(const QJsonObject &data)
{
    QMap<QString, QVariantList> sql;
    QString sheetName       = "drivers";
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

    sql = googleDataToSQL(true, dataOrder, data);
    bridgeDB->addSQLInfo(sheetName, creationQuery);
    bridgeDB->SQLDataInsert(sheetName, sql);

    dataGatheringJobs_.remove("drivers");
    beginAnalysis();
}

void Bridge::handleMRSDataPowerUnits(const QJsonObject &data)
{
    QMap<QString, QVariantList> sql;
    QString sheetName       = "powerUnits";

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

    sql = googleDataToSQL(true, dataOrder, data);
    bridgeDB->addSQLInfo(sheetName, creationQuery);
    bridgeDB->SQLDataInsert(sheetName, sql);

    dataGatheringJobs_.remove("powerUnits");
    beginAnalysis();
}

QMap<QString, QVariantList> Bridge::googleDataToSQL(bool hasHeader, const QStringList dataOrder, const QJsonObject &data)
{
    QMap<QString, QVariantList> sql;
    QJsonArray array;
    QJsonArray row;
    array = data["values"].toArray();
    for(auto valArr:array)
    {
        if(hasHeader)
        {
            hasHeader = false;
            continue;
        }

        row = valArr.toArray();
        for(int i = 0; i < dataOrder.size(); ++i)
        {
            if(row.size() <= i)
                sql[dataOrder[i]].append(QVariant());

            else if(row[i].toString().isEmpty())
                sql[dataOrder[i]].append(QVariant());

            else
                sql[dataOrder[i]].append(row[i].toVariant());
        }
    }
    return sql;
}
