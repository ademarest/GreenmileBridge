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

    //    connect(gmConn, &GMConnection::downloadProgess, this, &Bridge::downloadProgess);
    //    connect(gmConn, &GMConnection::statusMessage, this, &Bridge::statusMessage);
    //    connect(mrsConn, &MRSConnection::statusMessage, this, &Bridge::statusMessage);
    //    connect(mrsConn, &MRSConnection::routeSheetData, this, &Bridge::handleMasterRouteSheetData);


    //    connect(mrsConn, &MRSConnection::mrsDailyScheduleSQL, this, &Bridge::handleMRSDailyScheduleSQL);
}


void Bridge::startBridge()
{
    mrsConn->requestRouteKeysForDate("SEATTLE", QDate::currentDate());
    mrsDataConn->requestValuesFromAGoogleSheet("routeStart", "routeStart");
    as400Conn->getRouteDataForGreenmile(QDate::currentDate(), 10000);
    gmConn->requestAllOrganizationInfo();
    gmConn->requestRouteComparisonInfo(QDate::currentDate());
    gmConn->requestLocationInfo();
}

void Bridge::handleRouteQueryResults(const QMap<QString, QVariantList> &sql)
{
    emit statusMessage("Route info retrieved from AS400. There's " + QString::number(sql["route:key"].size()) + " stops for all Charlie's divisions" + QStringList(sql.keys()).join(", "));

    QString as400RouteQueryTableName    = "as400RouteQuery";
    QString as400ROuteQueryCreationQuery = "CREATE TABLE `as400RouteQuery` "
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
                                           "`stop:plannedSequenceNumber` INT, "
                                           "PRIMARY KEY(`order:number`))";

    bridgeDB->addSQLInfo(as400RouteQueryTableName, as400ROuteQueryCreationQuery);
    bridgeDB->SQLDataInsert("as400RouteQuery", sql);
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
                                          "stops"};

    bridgeDB->addJsonArrayInfo(gmRouteQueryTableName, gmRouteQueryCreationQuery, gmRouteQueryExpectedKeys);
    bridgeDB->JSONArrayInsert("gmRoutes", array);
}

void Bridge::routeMRSDataToFunction(const QString &key, const QJsonObject &data)
{
    emit statusMessage("MRS Data for " + key + " retrieved.");
    if(key == "routeStart")
        handleMRSDataRouteStartTimes(data);
}


void Bridge::handleMRSDataRouteStartTimes(const QJsonObject &data)
{
    QMap<QString, QVariantList> sql;
    QString gmRouteQueryTableName       = "routeStart";
    //ROUTE	AVG STARTS PREV	AVG START TIME	MON	STARTS PREV DAY MON	TUE	STARTS PREV DAY TUE	WED	STARTS PREV DAY WED	THU	STARTS PREV DAY THU	FRI	STARTS PREV DAY FRI	SAT	STARTS PREV DAY SAT	SUN	STARTS PREV DAY SUN
    QString gmRouteQueryCreationQuery = "CREATE TABLE `routeStart` "
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
    bridgeDB->addSQLInfo(gmRouteQueryTableName, gmRouteQueryCreationQuery);
    bridgeDB->SQLDataInsert(gmRouteQueryTableName, sql);
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
/*
void Bridge::stopBridge()
{

}

void Bridge::handleMasterRouteSheetData(QJsonObject sheetData)
{
    //qDebug() << sheetData;
    seattleMRSDailyScheduleToCommonForm(sheetData);
}

bool Bridge::uploadRoutes()
{
    return false;
}


void Bridge::handleRouteKeysForDate(QJsonArray routeArray)
{
    emit statusMessage("Today's route list recieved from Greenmile. There are "
                       + QString::number(routeArray.size())
                       + " routes uploaded for "
                       + QDate::currentDate().toString(Qt::ISODate)
                       + ".");
}

void Bridge::handleRouteComparisonInfo(QJsonArray routeArray)
{
    emit statusMessage("Today's route comarison result recieved from Greenmile. There are "
                       + QString::number(routeArray.size())
                       + " routes uploaded for "
                       + QDate::currentDate().toString(Qt::ISODate)
                       + ".");

    bridgeDB->handleGMRouteQuery(routeArray);
    gmRouteComparisonInfoToCommonForm(routeArray);
}

void Bridge::handleLocationKeys(QJsonArray locationArray)
{
    emit statusMessage("Locations retrieved from Greenmile. There are "
                       + QString::number(locationArray.size())
                       + " locations.");

    gmLocationInfoToCommonFormat(locationArray);
}

void Bridge::handleGMLocationInfo(QJsonArray locationArray)
{
    emit statusMessage("Locations info retrieved from Greenmile. There are "
                       + QString::number(locationArray.size())
                       + " locations.");
    bridgeDB->handleGMLocationInfo(locationArray);

    gmLocationInfoDone_ = true;
    if(gmLocationKeysDone_ &&
    gmOrganizationInfoDone_ &&
    gmRouteComparisonInfoDone_ &&
    as400RouteQueryDone_ &&
    mrsRouteDataDone_ &&
    mrsDataDriverDone_ &&
    mrsDataEquipmentDone_ &&  gmLocationInfoDone_)
    {
        makeRoutesToUpload();
    }
}



void Bridge::handleAllGreenmileOrgInfoResults(QJsonArray organizationInfo)
{
    QStringList orgKeys;
    for(auto org:organizationInfo)
        orgKeys.append(org.toObject()["key"].toString());

    emit statusMessage("Organization info retrieved from Greenmile. There's "
                       + QString::number(organizationInfo.size())
                       + " organizations for all Charlie's divisions " + orgKeys.join(", "));

    bridgeDB->handleGMOrganizationQuery(organizationInfo);
    gmOrganizationInfoToCommonForm(organizationInfo);
}

void Bridge::handleMasterRouteDataResults(const QString &key, const QJsonObject &sheetData)
{
    if(key == "powerUnit")
    {
        mrsDataEquipmentToCommonForm(sheetData);
    }

    if(key == "driver")
    {
        mrsDataDriverToCommonForm(sheetData);
    }
}

void Bridge::handleMRSDailyScheduleSQL(const QMap<QString, QVariantList> sql)
{
    bridgeDB->handleMRSDailyAssignmentSQL(sql);
}

void Bridge::bridgeLoop()
{

}

void Bridge::as400RouteResultToCommonForm(const QMap<QString, QVariantList> &sqlResults)
{
    QDate routeDate;
    QString routeKey;
    QString organizationKey;
    QString locationKey;
    int stopSequence;

    if(sqlResults.isEmpty())
    {
        emit statusMessage("Empty route result from AS400 for Greenmile. Cannot upload to Greenmile.");
        return;
    }
    for(auto key:sqlResults.keys())
    {
        if(sqlResults[key].size() != sqlResults.first().size())
        {
            emit statusMessage("Route results from AS400 have different rows for each column. Cannot upload to Greenmile.");
            return;
        }
    }

    for(int i = 0; i < sqlResults.first().size(); ++i)
    {
        //qDebug() << "init";
        organizationKey = sqlResults["organization:key"][i].toString();
        routeKey        = sqlResults["route:key"][i].toString();
        routeDate       = sqlResults["route:date"][i].toDate();
        locationKey     = sqlResults["location:key"][i].toString();
        stopSequence    = sqlResults["stop:plannedSequenceNumber"][i].toInt();

        //Make Organization
        //qDebug() << "org";
        QJsonObject organization;
        organization["key"] = QJsonValue(sqlResults["organization:key"][i].toString());
        as400Organizations_[organizationKey] = organization;

        //Make Driver
        //qDebug() << "drv";
        QJsonObject driver = as400Driver_[organizationKey][routeDate][routeKey];
        driver["key"] = QJsonValue(sqlResults["driver:key"][i].toString());
        driver["organization"] = organization;
        as400Driver_[organizationKey][routeDate][routeKey] = driver;

        //Make Equipment
        //qDebug() << "eqp";
        QJsonObject equipment = as400Equipment_[organizationKey][routeDate][routeKey];
        equipment["key"] = QJsonValue(sqlResults["equipment:key"][i].toString());
        equipment["organization"] = organization;
        as400Equipment_[organizationKey][routeDate][routeKey] = equipment;

        //Make Location
        //qDebug() << "loc";
        QJsonObject location;
        location["addressLine1"] = QJsonValue(sqlResults["location:addressLine1"][i].toString());
        location["addressLine2"] = QJsonValue(sqlResults["location:addressLine2"][i].toString());
        location["city"] = QJsonValue(sqlResults["location:city"][i].toString());
        location["deliveryDays"] = QJsonValue(sqlResults["location:deliveryDays"][i].toString());
        location["description"] = QJsonValue(sqlResults["location:description"][i].toString());
        location["key"] = QJsonValue(sqlResults["location:key"][i].toString());
        location["state"] = QJsonValue(sqlResults["location:state"][i].toString());
        location["zipCode"] = QJsonValue(sqlResults["location:zipCode"][i].toString());
        location["organization"] = organization;
        as400Locations_[organizationKey][routeDate][routeKey][locationKey] = location;

        //Make Stop
        //qDebug() << "stop";
        QJsonObject stop = as400Stops_[organizationKey][routeDate][routeKey][stopSequence];
        stop["key"]  = QJsonValue(QUuid::createUuid().toString());
        stop["plannedSequenceNumber"] = QJsonValue(sqlResults["stop:plannedSequenceNumber"][i].toInt());
        stop["location"] = QJsonObject {{"key", location["key"]}};
    QJsonArray stopOrders = stop["orders"].toArray();

    //Make Order
    //qDebug() << "order";
    QJsonObject order;
    order["number"]        = QJsonValue(sqlResults["order:number"][i].toString());
    order["plannedSize1"] = QJsonValue(sqlResults["order:pieces"][i].toDouble());
    order["plannedSize2"] = QJsonValue(sqlResults["order:cube"][i].toDouble());
    order["plannedSize3"] = QJsonValue(sqlResults["order:weight"][i].toDouble());
    stopOrders.append(order);
    stop["orders"] = stopOrders;
    as400Stops_[organizationKey][routeDate][routeKey][stopSequence] = stop;

    //qDebug() << "two";
    QJsonObject two;
    QString dowDeliver = sqlResults["location:deliveryDays"][i].toString();

    two["closeTime"] = QJsonValue(sqlResults["locationOverrideTimeWindows:closeTime"][i].toString());
    two["openTime"] = QJsonValue(sqlResults["locationOverrideTimeWindows:openTime"][i].toString());
    two["tw1Close"] = QJsonValue(sqlResults["locationOverrideTimeWindows:tw1Close"][i].toString());
    two["tw1Open"] = QJsonValue(sqlResults["locationOverrideTimeWindows:tw1Open"][i].toString());
    two["tw2Close"] = QJsonValue(sqlResults["locationOverrideTimeWindows:tw2Close"][i].toString());
    two["tw2Open"] = QJsonValue(sqlResults["locationOverrideTimeWindows:tw2Open"][i].toString());

    if(dowDeliver.contains("M"))
        two["monday"] = QJsonValue(true);
    if(dowDeliver.contains("T"))
        two["tuesday"] = QJsonValue(true);
    if(dowDeliver.contains("W"))
        two["wednesday"] = QJsonValue(true);
    if(dowDeliver.contains("R"))
        two["thursday"] = QJsonValue(true);
    if(dowDeliver.contains("F"))
        two["friday"] = QJsonValue(true);
    if(dowDeliver.contains("S"))
        two["saturday"] = QJsonValue(true);
    if(dowDeliver.contains("U"))
        two["sunday"] = QJsonValue(true);

    as400LocationTimeWindowOverrides_[organizationKey][routeDate][routeKey][locationKey] = two;

    //Make Route
    //qDebug() << "route";
    QJsonObject route;
    route = as400Route_[organizationKey][routeDate][routeKey];
    route["date"] = QJsonValue(routeDate.toString(Qt::ISODate));
    route["key"]  = QJsonValue(routeKey);
    route["organization"]  = organization;
    QJsonArray stops;

    for(auto stop:as400Stops_[organizationKey][routeDate][routeKey])
        stops.append(stop);

    route["stops"] = stops;
    as400Route_[organizationKey][routeDate][routeKey] = route;
}


as400RouteQueryDone_ = true;
if(gmLocationKeysDone_ &&
gmOrganizationInfoDone_ &&
gmRouteComparisonInfoDone_ &&
as400RouteQueryDone_ &&
mrsRouteDataDone_ &&
mrsDataDriverDone_ &&
mrsDataEquipmentDone_ &&  gmLocationInfoDone_)
{
    makeRoutesToUpload();
}
}

void Bridge::gmOrganizationInfoToCommonForm(const QJsonArray &organizationInfo)
{
    if(organizationInfo.isEmpty())
    {
        emit errorMessage("Greenmile Organization response is empty. Aborting.");
        return;
    }

    for(auto org:organizationInfo)
    {
        QString orgKey = org.toObject()["key"].toString();
        gmOrganizations_[orgKey] = org.toObject();
    }

    gmOrganizationInfoDone_ = true;
    if(gmLocationKeysDone_ &&
            gmOrganizationInfoDone_ &&
            gmRouteComparisonInfoDone_ &&
            as400RouteQueryDone_ &&
            mrsRouteDataDone_ &&
            mrsDataDriverDone_ &&
            mrsDataEquipmentDone_ &&  gmLocationInfoDone_)
    {
        makeRoutesToUpload();
    }
}

void Bridge::gmRouteComparisonInfoToCommonForm(const QJsonArray &routeComparisonInfo)
{
    if(routeComparisonInfo.isEmpty())
    {
        emit errorMessage("Greenmile Route Comparison response is empty. Aborting.");
        gmRouteComparisonInfoDone_ = true;

        if(gmLocationKeysDone_ &&
                gmOrganizationInfoDone_ &&
                gmRouteComparisonInfoDone_ &&
                as400RouteQueryDone_ &&
                mrsRouteDataDone_ &&
                mrsDataDriverDone_ &&
                mrsDataEquipmentDone_ &&  gmLocationInfoDone_)
        {
            makeRoutesToUpload();
        }
        return;
    }

    QString orgKey;
    QDate routeDate;
    QString routeKey;
    QJsonObject route;
    QJsonObject stop;
    QString locationKey;
    QJsonObject location;
    QJsonArray twoArray;


    for(auto routeVal:routeComparisonInfo)
    {
        route = routeVal.toObject();

        orgKey      = route["organization"].toObject()["key"].toString();
        routeDate   = QDate::fromString(route["date"].toString(), Qt::ISODate);
        routeKey    = route["key"].toString();

        gmRoute_[orgKey][routeDate][routeKey]                = route;
        gmDriverAssignments_[orgKey][routeDate][routeKey]    = route["driverAssignments"].toArray();
        gmEquipmentAssignments_[orgKey][routeDate][routeKey] = route["equipmentAssignments"].toArray();

        for(auto stopVal:route["stops"].toArray())
        {
            stop = stopVal.toObject();
            location = stop["location"].toObject();
            locationKey = location["key"].toString();
            gmRouteLocations_[orgKey][routeDate][routeKey][locationKey] = location;
            twoArray = location["locationOverrideTimeWindows"].toArray();

            if(twoArray.isEmpty())
                gmRouteLocationTimeWindowOverrides_[orgKey][routeDate][routeKey][locationKey] = QJsonObject();
            else
                gmRouteLocationTimeWindowOverrides_[orgKey][routeDate][routeKey][locationKey] = twoArray.first().toObject();
        }
    }

    gmRouteComparisonInfoDone_ = true;

    if(gmLocationKeysDone_ &&
            gmOrganizationInfoDone_ &&
            gmRouteComparisonInfoDone_ &&
            as400RouteQueryDone_ &&
            mrsRouteDataDone_ &&
            mrsDataDriverDone_ &&
            mrsDataEquipmentDone_ &&  gmLocationInfoDone_)
    {
        makeRoutesToUpload();
    }
}

void Bridge::gmLocationInfoToCommonFormat(const QJsonArray &locationInfo)
{
    QJsonObject location;
    QJsonObject organization;
    QString orgKey;
    QString locationKey;

    for(auto locationVal:locationInfo)
    {
        location = locationVal.toObject();
        locationKey = location["key"].toString();
        organization = location["organization"].toObject();
        orgKey = organization["key"].toString();
        gmLocations_[orgKey][locationKey] = location;
    }

    gmLocationKeysDone_ = true;
    if(gmLocationKeysDone_ &&
            gmOrganizationInfoDone_ &&
            gmRouteComparisonInfoDone_ &&
            as400RouteQueryDone_ &&
            mrsRouteDataDone_ &&
            mrsDataDriverDone_ &&
            mrsDataEquipmentDone_ &&  gmLocationInfoDone_)
    {
        makeRoutesToUpload();
    }
}

void Bridge::seattleMRSDailyScheduleToCommonForm(const QJsonObject &sheetData)
{
    bool foundDate = false;
    QString dateFormat = "d-MMM-yyyy";
    QString orgKey = "SEATTLE";
    QDate date = QDate::fromString(sheetData["routeDate"].toString(), Qt::ISODate);
    QString routeKey;
    QJsonObject route;
    QJsonObject driver;
    QJsonObject equipment;
    QJsonObject organization;
    int driverOffset = 1;
    int truckOffset = 2;
    int trailerOffset = 3;

    organization["key"] = orgKey;
    mrsOrganizations_[orgKey] = organization;

    //implement offset math
    //qDebug() << sheetData;
    QJsonArray rows = sheetData["values"].toArray();

    //find MRS date;
    for(auto rowArr:rows)
    {
        if(foundDate)
            break;

        QJsonArray row = rowArr.toArray();
        for(int i = 0; i<row.size(); ++i)
        {
            QString value = row[i].toString();
            QDate dateTest = QDate::fromString(value, dateFormat);
            if(dateTest.isValid() && !dateTest.isNull())
            {
                foundDate = true;
                date = dateTest;
                break;
            }
        }
    }

    for(auto rowArr:rows)
    {
        QJsonArray row = rowArr.toArray();
        for(int i = 0; i<row.size(); ++i)
        {
            QString value = row[i].toString();
            QRegularExpression routeRegExp("^[A-Z]-[A-Z,0-9]{3}");
            QRegularExpressionMatch match = routeRegExp.match(value);
            if(match.hasMatch())
            {
                routeKey = match.captured(0);
                route["key"] = QJsonValue(routeKey);
                route["date"] = QJsonValue(date.toString(Qt::ISODate));
                mrsRoute_[orgKey][date][routeKey] = route;
                if(i+driverOffset < row.size())
                {
                    driver["name"] = QJsonValue(row[i+driverOffset].toString());
                    mrsDriver_[orgKey][date][routeKey].append(driver);
                }
                if(i+truckOffset < row.size())
                {
                    if((row[i+truckOffset].toString() != "") && (row[i+truckOffset].toString().trimmed().toInt() != 0))
                    {
                        equipment["key"] = QJsonValue(row[i+truckOffset].toString().simplified());
                        mrsEquipment_[orgKey][date][routeKey].append(equipment);
                    }
                }
                if(i+trailerOffset < row.size())
                {
                    if((row[i+trailerOffset].toString() != "") && (row[i+trailerOffset].toString().trimmed().toInt() != 0))
                    {
                        equipment["key"] = QJsonValue(row[i+trailerOffset].toString().simplified());
                        mrsEquipment_[orgKey][date][routeKey].append(equipment);
                    }
                }
            }
        }
    }

    mrsRouteDataDone_ = true;
    if(gmLocationKeysDone_ &&
            gmOrganizationInfoDone_ &&
            gmRouteComparisonInfoDone_ &&
            as400RouteQueryDone_ &&
            mrsRouteDataDone_ &&
            mrsDataDriverDone_ &&
            mrsDataEquipmentDone_ &&  gmLocationInfoDone_)
    {
        makeRoutesToUpload();
    }
}

void Bridge::mrsDataDriverToCommonForm(const QJsonObject &sheetData)
{
    QJsonArray rows = sheetData["values"].toArray();
    mrsDataDriverDone_ = true;
    if(gmLocationKeysDone_ &&
            gmOrganizationInfoDone_ &&
            gmRouteComparisonInfoDone_ &&
            as400RouteQueryDone_ &&
            mrsRouteDataDone_ &&
            mrsDataDriverDone_ &&
            mrsDataEquipmentDone_ &&  gmLocationInfoDone_)
    {
        makeRoutesToUpload();
    }
}

void Bridge::mrsDataEquipmentToCommonForm(const QJsonObject &sheetData)
{
    QJsonArray rows = sheetData["values"].toArray();
    mrsDataEquipmentDone_ = true;
    if(gmLocationKeysDone_ &&
            gmOrganizationInfoDone_ &&
            gmRouteComparisonInfoDone_ &&
            as400RouteQueryDone_ &&
            mrsRouteDataDone_ &&
            mrsDataDriverDone_ &&
            mrsDataEquipmentDone_ &&  gmLocationInfoDone_)
    {
        makeRoutesToUpload();
    }
}

void Bridge::makeRoutesToUpload()
{
    //This whole class will be gutted and swapped to a database engine.
    //get common organizations

    //qDebug() << "hep;";
    QJsonObject uploadRoute;
    QList<QString> commonOrgNamesAS400_MRS;
    QMap<QString,QList<QDate>> commonRouteDatesAS400_MRS;
    QMap<QString,QMap<QDate,QList<QString>>> commonRouteKeysAS400_MRS;
    QMap<QString,QMap<QDate,QList<QString>>> gmRoutesToUpdate;
    QMap<QString,QMap<QDate,QList<QString>>> gmRoutesToUpload;

    for(auto as400OrgName:as400Organizations_.keys())
        if(mrsOrganizations_.contains(as400OrgName))
            commonOrgNamesAS400_MRS.append(as400OrgName);

    if(commonOrgNamesAS400_MRS.isEmpty())
    {
        emit errorMessage("No common organizations between AS400 and Google Sheets, aborting.");
        return;
    }

    for(auto orgName:commonOrgNamesAS400_MRS)
        for(auto date: as400Route_[orgName].keys())
            if(mrsRoute_[orgName].contains(date))
                commonRouteDatesAS400_MRS[orgName].append(date);

    if(commonRouteDatesAS400_MRS.isEmpty())
    {
        emit errorMessage("No common route dates between AS400 and Google Sheets, aborting.");
        return;
    }

    for(auto orgName:commonOrgNamesAS400_MRS)
        for(auto date: commonRouteDatesAS400_MRS[orgName])
            for(auto routeKey:as400Route_[orgName][date].keys())
                if(mrsRoute_[orgName][date].contains(routeKey))
                    commonRouteKeysAS400_MRS[orgName][date].append(routeKey);

    for(auto orgKey:commonRouteKeysAS400_MRS.keys())
    {
        if(gmOrganizations_.contains(orgKey))
        {
            for(auto date: commonRouteKeysAS400_MRS[orgKey].keys())
            {
                if(gmRoute_[orgKey].contains(date))
                {
                    for(auto routeKey: commonRouteKeysAS400_MRS[orgKey][date])
                    {
                        if(gmRoute_[orgKey][date].contains(routeKey))
                        {
                            gmRoutesToUpdate[orgKey][date].append(routeKey);
                        }
                        else
                        {
                            QJsonArray stops;
                            QJsonObject location;
                            gmRoutesToUpload[orgKey][date].append(routeKey);
                            as400Route_[orgKey][date][routeKey]["plannedArrival"] = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                            as400Route_[orgKey][date][routeKey]["plannedDeparture"] = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                            as400Route_[orgKey][date][routeKey]["plannedComplete"] = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                            as400Route_[orgKey][date][routeKey]["plannedStart"] = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                            as400Route_[orgKey][date][routeKey]["origin"] = QJsonObject({{"id",QJsonValue(10000)}});
                            as400Route_[orgKey][date][routeKey]["destination"] = QJsonObject({{"id",QJsonValue(10000)}});
                            as400Route_[orgKey][date][routeKey]["organization"] = QJsonObject({{"id", gmOrganizations_[orgKey]["id"]}});
                            as400Route_[orgKey][date][routeKey]["lastStopIsDestination"] = QJsonValue(false);
                            as400Route_[orgKey][date][routeKey]["hasHelper"] = QJsonValue(false);


                            for(auto stopVal:as400Route_[orgKey][date][routeKey]["stops"].toArray())
                            {
                                QJsonObject stop = stopVal.toObject();
                                stop["stopType"] = QJsonObject({{"id", QJsonValue(10000)}});
                                stops.append(stop);
                            }
                            as400Route_[orgKey][date][routeKey]["stops"] = QJsonValue(stops);
                            gmConn->uploadARoute(as400Route_[orgKey][date][routeKey]);
                            //qDebug() << QJsonDocument(as400Route_[orgKey][date][routeKey]).toJson(QJsonDocument::Compact);
                        }
                    }
                }
                else
                {
                    for(auto routeKey: commonRouteKeysAS400_MRS[orgKey][date])
                    {
                        QJsonArray stops;
                        QJsonObject location;
                        gmRoutesToUpload[orgKey][date].append(routeKey);
                        as400Route_[orgKey][date][routeKey]["plannedArrival"] = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                        as400Route_[orgKey][date][routeKey]["plannedDeparture"] = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                        as400Route_[orgKey][date][routeKey]["plannedComplete"] = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                        as400Route_[orgKey][date][routeKey]["plannedStart"] = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                        as400Route_[orgKey][date][routeKey]["origin"] = QJsonObject({{"id",QJsonValue(10000)}});
                        as400Route_[orgKey][date][routeKey]["destination"] = QJsonObject({{"id",QJsonValue(10000)}});
                        as400Route_[orgKey][date][routeKey]["organization"] = QJsonObject({{"id", gmOrganizations_[orgKey]["id"]}});
                        as400Route_[orgKey][date][routeKey]["lastStopIsDestination"] = QJsonValue(false);
                        as400Route_[orgKey][date][routeKey]["hasHelper"] = QJsonValue(false);


                        for(auto stopVal:as400Route_[orgKey][date][routeKey]["stops"].toArray())
                        {
                            QJsonObject stop = stopVal.toObject();
                            stop["stopType"] = QJsonObject({{"id", QJsonValue(10000)}});
                            stops.append(stop);
                        }
                        as400Route_[orgKey][date][routeKey]["stops"] = QJsonValue(stops);
                        gmConn->uploadARoute(as400Route_[orgKey][date][routeKey]);
                        //qDebug() << QJsonDocument(as400Route_[orgKey][date][routeKey]).toJson(QJsonDocument::Compact);
                    }
                }
            }
        }
    }

    //qDebug() << "upload" << gmRoutesToUpload;
    //qDebug() << "update" << gmRoutesToUpdate;
    //qDebug() << uploadRoute;
}
*/
