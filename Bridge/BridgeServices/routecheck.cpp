#include "routecheck.h"

RouteCheck::RouteCheck(QObject *parent) : QObject(parent)
{
    connect(gmConn_, &GMConnection::gmNetworkResponse, this, &RouteCheck::handleGMResponse);
    connect(gmConn_, &GMConnection::statusMessage, this, &RouteCheck::statusMessage);
    connect(gmConn_, &GMConnection::errorMessage, this, &RouteCheck::errorMessage);
    connect(gmConn_, &GMConnection::debugMessage, this, &RouteCheck::debugMessage);

    //connect(bridgeDB_, &BridgeDatabase::statusMessage, this, &RouteCheck::statusMessage);
    connect(bridgeDB_, &BridgeDatabase::errorMessage, this, &RouteCheck::errorMessage);
    connect(bridgeDB_, &BridgeDatabase::debugMessage, this, &RouteCheck::debugMessage);
}

RouteCheck::~RouteCheck()
{

}

void RouteCheck::reset()
{
    if(!activeJobs_.isEmpty())
    {
        errorMessage("Route deletion in progress. Try again once current request is finished.");
        qDebug() << "Route deletion in progress. Try again once current request is finished.";
        return;
    }

    currentKey_.clear();
    activeJobs_.clear();
    currentRequest_.clear();
    routesToDelete_ = QJsonObject();
    deletedRoutes_ = QJsonObject();
}

void RouteCheck::deleteIncorrectRoutes(const QString &key, const QList<QVariantMap> &argList)
{

    if(!activeJobs_.isEmpty())
    {
        errorMessage("Route deletion in progress. Try again once current requests are finished.");
        qDebug() << "Route deletion in progress. Try again once current requests are finished.";
        return;
    }

    currentKey_ = key;
    deletedRoutes_.empty();
    routesToDelete_.empty();

    for(auto vMap:argList)
    {
        QString tableName = vMap["tableName"].toString();
        QString organizationKey = vMap["organization:key"].toString();
        QDate date = vMap["date"].toDate();

        mergeRoutesToDelete(bridgeDB_->getRoutesToDelete(tableName, organizationKey, date));
    }

    for(auto key:routesToDelete_.keys())
    {
        if(key.split(":").first() == "DeleteRoute")
        {
            activeJobs_.insert(key);
            gmConn_->deleteRoute(key, routesToDelete_[key].toString());
        }
    }

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, QJsonObject());
        reset();
    }
}

void RouteCheck::handleGMResponse(const QString &key, const QJsonValue &response)
{
    activeJobs_.remove(key);
    deletedRoutes_[key] = response;
    qDebug() << activeJobs_;

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, deletedRoutes_);
        reset();
    }
}

void RouteCheck::mergeRoutesToDelete(const QJsonObject &routeIDs)
{
    qDebug() << routeIDs;
    for(auto key:routeIDs.keys())
    {
        routesToDelete_[key] = routeIDs[key];
    }
    qDebug() << routesToDelete_;
}
