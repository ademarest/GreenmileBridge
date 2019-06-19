#include "routeupload.h"

RouteUpload::RouteUpload(QObject *parent) : QObject(parent)
{
    connect(gmConn_, &GMConnection::networkResponse, this, &RouteUpload::handleGMResponse);
    connect(gmConn_, &GMConnection::statusMessage, this, &RouteUpload::statusMessage);
    connect(gmConn_, &GMConnection::errorMessage, this, &RouteUpload::errorMessage);
    connect(gmConn_, &GMConnection::debugMessage, this, &RouteUpload::debugMessage);

    connect(bridgeDB_, &BridgeDatabase::statusMessage, this, &RouteUpload::statusMessage);
    connect(bridgeDB_, &BridgeDatabase::errorMessage, this, &RouteUpload::errorMessage);
    connect(bridgeDB_, &BridgeDatabase::debugMessage, this, &RouteUpload::debugMessage);
}

RouteUpload::~RouteUpload()
{

}

void RouteUpload::reset()
{
    if(!activeJobs_.isEmpty())
    {
        errorMessage("Route upload in progress. Try again once current request is finished.");
        qDebug() << "Route upload in progress. Try again once current request is finished.";
        return;
    }

    currentKey_.clear();
    activeJobs_.clear();
    currentRequest_.clear();
    routesToUpload_ = QJsonObject();
    uploadedRoutes_ = QJsonObject();
}

void RouteUpload::UploadRoutes(const QString &key, const QList<QVariantMap> &argList)
{
    if(!activeJobs_.isEmpty())
    {
        errorMessage("Route upload in progress. Try again once current request is finished.");
        qDebug() << "Route upload in progress. Try again once current request is finished.";
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
        QString stopTypeId = vMap["stopTypeId"].toString();

        mergeRoutesToUpload(bridgeDB_->getRoutesToUpload(tableName, organizationKey, date, stopTypeId, minRouteKey, maxRouteKey));
    }

    for(auto key:routesToUpload_.keys())
    {

        if(key.split(":").first() == "routeUpload")
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

void RouteUpload::handleGMResponse(const QString &key, const QJsonValue &response)
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

void RouteUpload::mergeRoutesToUpload(const QJsonObject &locations)
{
    for(auto key:locations.keys())
    {
        routesToUpload_[key] = locations[key];
        qDebug() <<  locations[key];
    }
}
