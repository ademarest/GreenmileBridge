#include "bridge.h"

Bridge::Bridge(QObject *parent) : QObject(parent)
{
    connect(gmConn, &GMConnection::downloadProgess, this, &Bridge::downloadProgess);
    connect(gmConn, &GMConnection::routeKeysForDate, this, &Bridge::handleRouteKeysForDate);
    connect(gmConn, &GMConnection::locationKeys, this, &Bridge::handleLocationKeys);
    connect(gmConn, &GMConnection::statusMessage, this, &Bridge::statusMessage);
    connect(mrsConn, &MRSConnection::statusMessage, this, &Bridge::statusMessage);
}

void Bridge::startBridge()
{
    gmConn->requestRouteKeysForDate(QDate::currentDate());
    gmConn->requestLocationKeys();
    mrsConn->requestRouteKeysForDate(QDate::currentDate());
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

void Bridge::bridgeLoop()
{

}
