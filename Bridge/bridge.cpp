#include "bridge.h"

Bridge::Bridge(QObject *parent) : QObject(parent)
{
    connect(gmConn, &GMConnection::downloadProgess, this, &Bridge::downloadProgess);
    connect(gmConn, &GMConnection::routeKeysForDate, this, &Bridge::handleRouteKeysForDate);
    connect(gmConn, &GMConnection::locationKeys, this, &Bridge::handleLocationKeys);
    connect(gmConn, &GMConnection::statusMessage, this, &Bridge::statusMessage);
    connect(mrsConn, &MRSConnection::statusMessage, this, &Bridge::statusMessage);
    connect(as400Conn, &AS400::greenmileRouteInfoResults, this, &Bridge::handleRouteQueryResults);
    connect(as400Conn, &AS400::debugMessage, this, &Bridge::statusMessage);
}

void Bridge::startBridge()
{
    gmConn->requestRouteKeysForDate(QDate::currentDate());
    gmConn->requestLocationKeys();
    mrsConn->requestRouteKeysForDate(QDate::currentDate());
    as400Conn->getRouteDataForGreenmile(QDate::currentDate(), 10000);
}

void Bridge::stopBridge()
{

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

    as400RouteResultToGMJson(sqlResults);
}

void Bridge::bridgeLoop()
{

}

void Bridge::as400RouteResultToGMJson(const QMap<QString, QVariantList> &sqlResults)
{


    qDebug() << "0";

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
    qDebug() << "2";
    for(auto key:sqlResults.keys())
    {
        if(sqlResults[key].size() != sqlResults.first().size())
        {
            emit statusMessage("Route results from AS400 have different rows for each column. Cannot upload to Greenmile.");
            return;
        }
    }

    qDebug() << "1";
    for(int i = 0; i < sqlResults.first().size(); ++i)
    {
        /*
         * driver:key,
         * equipment:key,
         * location:addressLine1,
         * location:addressLine2,
         * location:city,
         * location:deliveryDays,
         * location:description,
         * location:key,
         * location:state,
         * location:zipCode,
         * locationOverrideTimeWindowsTW1:closetime,
         * locationOverrideTimeWindowsTW1:openTime,
         * locationOverrideTimeWindowsTW1:tw1Close,
         * locationOverrideTimeWindowsTW1:tw1Open,
         * locationOverrideTimeWindowsTW1:tw2Close,
         * locationOverrideTimeWindowsTW1:tw2Open,
         * order:cube,
         * order:number,
         * order:pieces,
         * order:weight,
         * organization:key,
         * route:date,
         * route:key,
         * stop:baseLinePlannedSequence
        */
        qDebug() << "init";
        organizationKey = sqlResults["organization:key"][i].toString();
        routeKey        = sqlResults["route:key"][i].toString();
        routeDate       = sqlResults["route:date"][i].toDate();
        locationKey     = sqlResults["location:key"][i].toString();
        stopSequence    = sqlResults["stop:baseLineSequenceNum"][i].toInt();

        //Make Organization
        qDebug() << "org";
        QJsonObject organization;
        organization["key"] = QJsonValue(sqlResults["organization:key"][i].toString());

        //Make Driver
        qDebug() << "drv";
        QJsonObject driver = gmDriver_[organizationKey][routeDate][routeKey];
        driver["key"] = QJsonValue(sqlResults["driver:key"][i].toString());
        driver["organization"] = organization;
        gmDriver_[organizationKey][routeDate][routeKey] = driver;

        //Make Equipment
        qDebug() << "eqp";
        QJsonObject equipment = gmEquipment_[organizationKey][routeDate][routeKey];
        equipment["key"] = QJsonValue(sqlResults["equipment:key"][i].toString());
        equipment["organization"] = organization;
        gmEquipment_[organizationKey][routeDate][routeKey] = equipment;

        //Make Location
        qDebug() << "loc";
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
        gmLocations_[organizationKey][locationKey] = location;

        //Make Stop
        qDebug() << "stop";
        QJsonObject stop = gmStops_[organizationKey][routeDate][routeKey][stopSequence];
        stop["key"]  = QJsonValue(QUuid::createUuid().toString());
        stop["baseLineSequenceNum"] = QJsonValue(sqlResults["stop:baseLineSequenceNum"][i].toInt());
        stop["location"] = location;
        QJsonArray stopOrders = stop["orders"].toArray();

        //Make Order
        qDebug() << "order";
        QJsonObject order;
        order["number"]        = QJsonValue(sqlResults["order:number"][i].toString());
        order["baselineSize1"] = QJsonValue(sqlResults["order:pieces"][i].toDouble());
        order["baselineSize2"] = QJsonValue(sqlResults["order:cube"][i].toDouble());
        order["baselineSize3"] = QJsonValue(sqlResults["order:weight"][i].toDouble());
        stopOrders.append(order);
        stop["orders"] = stopOrders;
        gmStops_[organizationKey][routeDate][routeKey][stopSequence] = stop;

        //Make twOveride
        qDebug() << "two";
        QJsonObject locationTimeWindowOverride;

        //Make Route
        qDebug() << "route";
        QJsonObject route;
        route = gmRoute_[organizationKey][routeDate][routeKey];
        route["date"] = QJsonValue(routeDate.toString(Qt::ISODate));
        route["key"]  = QJsonValue(routeKey);
        route["organization"]  = organization;
        QJsonArray stops;
        for(auto stop:gmStops_[organizationKey][routeDate][routeKey])
            stops.append(stop);
        route["stops"] = stops;
        gmRoute_[organizationKey][routeDate][routeKey] = route;
        qDebug() << "dingo";
        qDebug() << QJsonDocument(gmRoute_[organizationKey][routeDate][routeKey]).toJson();
    }
}
