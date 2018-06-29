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
        QJsonObject organization;
        QJsonObject stop;
        QJsonObject location;
        QJsonObject order;
        QJsonObject driver;
        QJsonObject equipment;

        organization["key"] = QJsonValue(sqlResults["organization:key"][i].toString());
        stop["key"]  = QJsonValue(QUuid::createUuid().toString());
        stop["baseLineSequenceNumber"] = QJsonValue(sqlResults["stop:baseLineSequenceNumber"][i].toString());
        location["key"] = QJsonValue(sqlResults["location:key"][i].toString());
        location["organization"] = organization;
        location["description"] = QJsonValue(sqlResults["location:description"][i].toString());
        location["addressLine1"] = QJsonValue(sqlResults["location:addressLine1"][i].toString());
        location["addressLine2"] = QJsonValue(sqlResults["location:addressLine2"][i].toString());
        location["city"] = QJsonValue(sqlResults["location:city"][i].toString());
        location["state"] = QJsonValue(sqlResults["location:state"][i].toString());
        location["zipCode"] = QJsonValue(sqlResults["location:zipCode"][i].toString());
        order["number"] = QJsonValue(sqlResults["order:number"][i].toInt());
        order["baselineSize1"] = QJsonValue(sqlResults["order:pieces"][i].toInt());
        order["baselineSize2"] = QJsonValue(sqlResults["order:weight"][i].toInt());
        order["baselineSize3"] = QJsonValue(sqlResults["order:cube"][i].toInt());
        driver["key"] = QJsonValue(sqlResults["driver:key"][i].toString());
        driver["organization"] = organization;
        equipment["key"] = QJsonValue(sqlResults["equipment:key"][i].toString());
        equipment["organization"] = organization;

        routeDate   = sqlResults["route:date"][i].toDate();
        routeKey    = sqlResults["route:key"][i].toString();
        organizationKey = sqlResults["organization:key"][i].toString();

        gmRoute_[organizationKey][routeDate][routeKey]["date"] = QJsonValue(routeDate.toString(Qt::ISODate));
        gmRoute_[organizationKey][routeDate][routeKey]["key"]  = QJsonValue(routeKey);
        gmRoute_[organizationKey][routeDate][routeKey]["organization"]  = organization;

        stop["location"] = location;
        stop["orders"] = QJsonArray{order};
        QJsonArray stopArray = gmRoute_[organizationKey][routeDate][routeKey]["stops"].toArray();
        stopArray.append(stop);
        gmRoute_[organizationKey][routeDate][routeKey]["stops"] = QJsonValue(stopArray);
        gmDriver_[organizationKey][routeDate][routeKey] = driver;
        gmEquipment_[organizationKey][routeDate][routeKey] = equipment;
        qDebug() << "dingo";
        qDebug() << QJsonDocument(gmRoute_[organizationKey][routeDate][routeKey]).toJson();
    }
}
