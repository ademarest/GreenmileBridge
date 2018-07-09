#include "bridge.h"

Bridge::Bridge(QObject *parent) : QObject(parent)
{
    connect(bridgeDB, &BridgeDatabase::debugMessage, this, &Bridge::statusMessage);
    connect(bridgeDB, &BridgeDatabase::errorMessage, this, &Bridge::statusMessage);
    connect(bridgeDB, &BridgeDatabase::statusMessage, this, &Bridge::statusMessage);
//    connect(gmConn, &GMConnection::downloadProgess, this, &Bridge::downloadProgess);
//    connect(gmConn, &GMConnection::routeKeysForDate, this, &Bridge::handleRouteKeysForDate);
//    connect(gmConn, &GMConnection::locationKeys, this, &Bridge::handleLocationKeys);
//    connect(gmConn, &GMConnection::statusMessage, this, &Bridge::statusMessage);
//    connect(mrsConn, &MRSConnection::statusMessage, this, &Bridge::statusMessage);
//    connect(mrsConn, &MRSConnection::routeSheetData, this, &Bridge::handleMasterRouteSheetData);
    connect(as400Conn, &AS400::greenmileRouteInfoResults, this, &Bridge::handleRouteQueryResults);
    connect(as400Conn, &AS400::debugMessage, this, &Bridge::statusMessage);
//    connect(gmConn, &GMConnection::allOrganizationInfo, this, &Bridge::handleAllGreenmileOrgInfoResults);
//    connect(gmConn, &GMConnection::routeComparisonInfo, this, &Bridge::handleRouteComparisonInfo);
//    connect(mrsDataConn, &MRSDataConnection::data, this, &Bridge::handleMasterRouteDataResults);
//    connect(bridgeDB, &BridgeDatabase::debugMessage, this, &Bridge::statusMessage);
//    connect(gmConn, &GMConnection::gmLocationInfo, this, &Bridge::handleGMLocationInfo);
//    connect(mrsConn, &MRSConnection::mrsDailyScheduleSQL, this, &Bridge::handleMRSDailyScheduleSQL);
}


void Bridge::startBridge()
{
    bridgeDB->init();
    emit statusMessage("hep");
//    gmConn->requestRouteKeysForDate(QDate::currentDate());
//    gmConn->requestLocationKeys();
//    gmLocationKeysDone_ = false;
//    gmRouteComparisonInfoDone_ = false;
//    gmOrganizationInfoDone_ = false;
//    as400RouteQueryDone_ = false;
//    mrsRouteDataDone_ = false;
//    mrsDataDriverDone_ = false;
//    mrsDataEquipmentDone_ = false;
//    gmLocationInfoDone_ = false;

//    gmConn->requestLocaitonInfo();
//    gmConn->requestLocationKeys();
//    gmConn->requestAllOrganizationInfo();
//    gmConn->requestRouteComparisonInfo(QDate::currentDate());
//    mrsConn->requestRouteKeysForDate("SEATTLE", QDate::currentDate());
//    mrsDataConn->requestValuesFromAGoogleSheet("powerUnit", "powerUnit");
//    mrsDataConn->requestValuesFromAGoogleSheet("driver", "driver");

    as400Conn->getRouteDataForGreenmile(QDate::currentDate(), 10000);
}

void Bridge::handleRouteQueryResults(QMap<QString, QVariantList> sqlResults)
{
    emit statusMessage("Route info retrieved from AS400. There's "
                       + QString::number(sqlResults["route:key"].size())
            + " stops for all Charlie's divisions" + QStringList(sqlResults.keys()).join(", "));

    bridgeDB->SQLDataInsert("as400RouteQuery", sqlResults);
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
