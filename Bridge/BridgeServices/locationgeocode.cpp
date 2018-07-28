#include "locationgeocode.h"

LocationGeocode::LocationGeocode(QObject *parent) : QObject(parent)
{
    connect(gmConn_, &GMConnection::gmNetworkResponse, this, &LocationGeocode::handleGMResponse);

    connect(gmConn_, &GMConnection::statusMessage, this, &LocationGeocode::statusMessage);
    connect(gmConn_, &GMConnection::errorMessage, this, &LocationGeocode::errorMessage);
    connect(gmConn_, &GMConnection::debugMessage, this, &LocationGeocode::debugMessage);

    //connect(bridgeDB_, &BridgeDatabase::statusMessage, this, &LocationGeocode::statusMessage);
    connect(bridgeDB_, &BridgeDatabase::errorMessage, this, &LocationGeocode::errorMessage);
    connect(bridgeDB_, &BridgeDatabase::debugMessage, this, &LocationGeocode::debugMessage);
}


void LocationGeocode::reset()
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
    locationsToGeocode_ = QJsonObject();
    geocodedLocations_ = QJsonObject();
}

void LocationGeocode::GeocodeLocations(const QString &key, const QList<QVariantMap> &argList)
{

    if(!activeJobs_.isEmpty())
    {
        errorMessage("Geocoding in progress. Try again once current request is finished.");
        qDebug() << "Geocoding in progress. Try again once current request is finished.";
        return;
    }

    currentKey_ = key;
    geocodedLocations_.empty();
    locationsToGeocode_.empty();

    for(auto vMap:argList)
    {
        QString tableName = vMap["tableName"].toString();
        QString organizationKey = vMap["organization:key"].toString();
        QDate date = vMap["date"].toDate();
        QString minRouteKey = vMap["minRouteKey"].toString();
        QString maxRouteKey = vMap["maxRouteKey"].toString();

        mergeLocationsToGeocode(bridgeDB_->getLocationsToUpload(tableName, organizationKey, date, minRouteKey, maxRouteKey));
    }

    for(auto key:locationsToGeocode_.keys())
    {
        activeJobs_.insert(key);
        gmConn_->geocodeLocation(key, locationsToGeocode_[key].toObject());
    }

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, QJsonObject());
        reset();
    }

}

void LocationGeocode::handleGMResponse(const QString &key, const QJsonValue &response)
{
    activeJobs_.remove(key);
    geocodedLocations_[key] = response;

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, geocodedLocations_);
        reset();
    }
}

void LocationGeocode::mergeLocationsToGeocode(const QJsonObject &locations)
{
    for(auto key:locations.keys())
    {
        locationsToGeocode_[key] = locations[key];
    }
}
