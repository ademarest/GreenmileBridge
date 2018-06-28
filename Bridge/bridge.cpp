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
}

void Bridge::bridgeLoop()
{

}
