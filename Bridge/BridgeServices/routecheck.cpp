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

void RouteCheck::reset()
{
    if(!activeJobs_.isEmpty())
    {
        errorMessage("Geocoding in progress. Try again once current request is finished.");
        qDebug() << "Geocoding in progress. Try again once current request is finished.";
        return;
    }

    currentKey_.clear();
    activeJobs_.clear();
    currentRequest_.clear();
    routesToUpload_ = QJsonObject();
    uploadedRoutes_ = QJsonObject();
}

void RouteCheck::UploadRoutes(const QString &key, const QList<QVariantMap> &argList)
{

    if(!activeJobs_.isEmpty())
    {
        errorMessage("Geocoding in progress. Try again once current request is finished.");
        qDebug() << "Geocoding in progress. Try again once current request is finished.";
        return;
    }

    currentKey_ = key;
    uploadedRoutes_.empty();
    routesToUpload_.empty();

    for(auto vMap:argList)
    {
        QString tableName = vMap["tableName"].toString();
        QString organizationKey = vMap["organization:key"].toString();
        QDate date = vMap["date"].toDate();
        QString minRouteKey = vMap["minRouteKey"].toString();
        QString maxRouteKey = vMap["maxRouteKey"].toString();

        mergeRoutesToUpload(bridgeDB_->getRoutesToUpload(tableName, organizationKey, date, minRouteKey, maxRouteKey));
    }

    for(auto key:routesToUpload_.keys())
    {

        if(key.split(":").first() == "RouteCheck")
        {
            activeJobs_.insert(key);
            gmConn_->uploadARoute(key, routesToUpload_[key].toObject());
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
    uploadedRoutes_[key] = response;
    qDebug() << activeJobs_;

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, uploadedRoutes_);
        reset();
    }
}

void RouteCheck::mergeRoutesToUpload(const QJsonObject &locations)
{
    for(auto key:locations.keys())
    {
        routesToUpload_[key] = locations[key];
    }
}
