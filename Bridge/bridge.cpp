#include "bridge.h"

Bridge::Bridge(QObject *parent) : QObject(parent)
{
    connect(gmConn, &GMConnection::downloadProgess, this, &Bridge::downloadProgess);
    connect(gmConn, &GMConnection::routeKeysForDate, this, &Bridge::handleRouteKeysForDate);
    connect(gmConn, &GMConnection::locationKeys, this, &Bridge::handleLocationKeys);
    connect(gmConn, &GMConnection::statusMessage, this, &Bridge::statusMessage);
    connect(mrsConn, &MRSConnection::statusMessage, this, &Bridge::statusMessage);
    connect(mrsConn, &MRSConnection::routeSheetData, this, &Bridge::handleMasterRouteSheetData);
    connect(as400Conn, &AS400::greenmileRouteInfoResults, this, &Bridge::handleRouteQueryResults);
    connect(as400Conn, &AS400::debugMessage, this, &Bridge::statusMessage);
    connect(gmConn, &GMConnection::allOrganizationInfo, this, &Bridge::handleAllGreenmileOrgInfoResults);
    connect(gmConn, &GMConnection::routeComparisonInfo, this, &Bridge::handleRouteComparisonInfo);
}

void Bridge::startBridge()
{
    //gmConn->requestRouteKeysForDate(QDate::currentDate());
    //gmConn->requestLocationKeys();

    mrsConn->requestRouteKeysForDate(QDate::currentDate());
    gmOrganizationInfoDone_ = false;
    gmRouteComparisonInfoDone_ = false;
    as400RouteQueryDone_ = false;
    gmConn->requestAllOrganizationInfo();
    gmConn->requestRouteComparisonInfo(QDate::currentDate());
    as400Conn->getRouteDataForGreenmile(QDate::currentDate(), 10000);
}

void Bridge::stopBridge()
{

}

void Bridge::handleMasterRouteSheetData(QJsonObject sheetData)
{
    //qDebug() << sheetData;
    mrsKeysFromData(sheetData);
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

    gmRouteComparisonInfoToCommonForm(routeArray);
}

void Bridge::handleLocationKeys(QJsonArray locationArray)
{
    emit statusMessage("Locations retrieved from Greenmile. There are "
                       + QString::number(locationArray.size())
                       + " locations.");
}

void Bridge::handleRouteQueryResults(QMap<QString, QVariantList> sqlResults)
{
    emit statusMessage("Route info retrieved from AS400. There's "
                       + QString::number(sqlResults["route:key"].size())
            + " stops for all Charlie's divisions" + QStringList(sqlResults.keys()).join(", "));

    as400RouteResultToCommonForm(sqlResults);
}

void Bridge::handleAllGreenmileOrgInfoResults(QJsonArray organizationInfo)
{
    QStringList orgKeys;
    for(auto org:organizationInfo)
        orgKeys.append(org.toObject()["key"].toString());

    emit statusMessage("Organization info retrieved from Greenmile. There's "
                       + QString::number(organizationInfo.size())
            + " organizations for all Charlie's divisions " + orgKeys.join(", "));

    gmOrganizationInfoToCommonForm(organizationInfo);
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
        stopSequence    = sqlResults["stop:baseLineSequenceNum"][i].toInt();

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
        stop["baseLineSequenceNum"] = QJsonValue(sqlResults["stop:baseLineSequenceNum"][i].toInt());
        stop["location"] = QJsonObject {{"key", location["key"]}};
        QJsonArray stopOrders = stop["orders"].toArray();

        //Make Order
        //qDebug() << "order";
        QJsonObject order;
        order["number"]        = QJsonValue(sqlResults["order:number"][i].toString());
        order["baselineSize1"] = QJsonValue(sqlResults["order:pieces"][i].toDouble());
        order["baselineSize2"] = QJsonValue(sqlResults["order:cube"][i].toDouble());
        order["baselineSize3"] = QJsonValue(sqlResults["order:weight"][i].toDouble());
        stopOrders.append(order);
        stop["orders"] = stopOrders;
        as400Stops_[organizationKey][routeDate][routeKey][stopSequence] = stop;

        //qDebug() << "two";
        QJsonObject two;
        QString dowDeliver = sqlResults["location:deliveryDays"][i].toString();

        two["closeTime"] = QJsonValue(sqlResults["locationOverrideTimeWindowsTW1:closeTime"][i].toString());
        two["openTime"] = QJsonValue(sqlResults["locationOverrideTimeWindowsTW1:openTime"][i].toString());
        two["tw1Close"] = QJsonValue(sqlResults["locationOverrideTimeWindowsTW1:tw1Close"][i].toString());
        two["tw1Open"] = QJsonValue(sqlResults["locationOverrideTimeWindowsTW1:tw1Open"][i].toString());
        two["tw2Close"] = QJsonValue(sqlResults["locationOverrideTimeWindowsTW1:tw2Close"][i].toString());
        two["tw2Open"] = QJsonValue(sqlResults["locationOverrideTimeWindowsTW1:tw2Open"][i].toString());

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
    if(gmOrganizationInfoDone_ && gmRouteComparisonInfoDone_ && as400RouteQueryDone_)
        makeRoutesToUpload();

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
    if(gmOrganizationInfoDone_ && gmRouteComparisonInfoDone_ && as400RouteQueryDone_)
        makeRoutesToUpload();
}

void Bridge::gmRouteComparisonInfoToCommonForm(const QJsonArray &routeComparisonInfo)
{
    if(routeComparisonInfo.isEmpty())
    {
        emit errorMessage("Greenmile Route Comparison response is empty. Aborting.");
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
    if(gmOrganizationInfoDone_ && gmRouteComparisonInfoDone_ && as400RouteQueryDone_)
        makeRoutesToUpload();
}

void Bridge::mrsKeysFromData(const QJsonObject &sheetData)
{
    QSet<QString> matchSet;
    QStringList matchList;

    //qDebug() << sheetData;
    QJsonArray rows = sheetData["values"].toArray();
    for(auto rowArr:rows)
    {
        QJsonArray row = rowArr.toArray();
        for(auto value:row)
        {
            QRegularExpression routeRegExp("^[A-Z]-[A-Z,0-9]{3}");
            QRegularExpressionMatch match = routeRegExp.match(value.toString());
            if(match.hasMatch())
                matchSet.insert(match.captured(0));
        }
    }
    matchList = matchSet.toList();
    matchList.sort();
    qDebug() << matchList;
}

void Bridge::makeRoutesToUpload()
{

    //get common organizations
    QList<QString> commonOrgNames;
    QMap<QString,QList<QDate>> commonRouteDates;
    QMap<QString,QMap<QDate,QList<QString>>> commonRouteKeys;

    for(auto as400OrgName:as400Organizations_.keys())
        if(gmOrganizations_.contains(as400OrgName))
            commonOrgNames.append(as400OrgName);

    if(commonOrgNames.isEmpty())
    {
        emit errorMessage("No common organizations between AS400, Greenmile, and Google Sheets, aborting.");
        return;
    }

    for(auto orgName:commonOrgNames)
        for(auto date: as400Route_[orgName].keys())
            if(gmRoute_[orgName].contains(date))
                commonRouteDates[orgName].append(date);

    if(commonRouteDates.isEmpty())
    {
        emit errorMessage("No common route dates between AS400, Greenmile, and Google Sheets, aborting.");
        return;
    }

    for(auto orgName:commonOrgNames)
        for(auto date: commonRouteDates[orgName])
            for(auto routeKey:as400Route_[orgName][date].keys())
                if(gmRoute_[orgName][date].contains(routeKey))
                    commonRouteKeys[orgName][date].append(routeKey);

    qDebug() << "Keys: " << commonOrgNames << " Dates:" <<  commonRouteDates << "Route Keys: " << commonRouteKeys;

}
