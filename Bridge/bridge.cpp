#include "bridge.h"

Bridge::Bridge(QObject *parent) : QObject(parent)
{

}

void Bridge::startBridge()
{
    getRouteKeysFromGMForDate(QDate::currentDate());
}

bool Bridge::uploadRoutes()
{
    return false;
}

void Bridge::getRouteKeysFromGMForDate(const QDate &date)
{
    connect(gmConn, &GMConnection::downloadProgess, this, &Bridge::downloadProgess);
    connect(gmConn, &GMConnection::routeKeysForDate, this, &Bridge::handleRouteKeysForDate);
    gmConn->getRouteKeysForDate(date);
}


void Bridge::handleRouteKeysForDate(QJsonArray routeArray)
{
    disconnect(gmConn, &GMConnection::downloadProgess, this, &Bridge::downloadProgess);
    disconnect(gmConn, &GMConnection::routeKeysForDate, this, &Bridge::handleRouteKeysForDate);
    emit statusMessage("Today's route list recieved from Greenmile. There are "
                       + QString::number(routeArray.size())
                       + " routes uploaded for "
                       + QDate::currentDate().toString(Qt::ISODate)
                       + ".");
}
