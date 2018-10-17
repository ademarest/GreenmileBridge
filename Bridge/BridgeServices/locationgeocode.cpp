#include "locationgeocode.h"

LocationGeocode::LocationGeocode(QObject *parent) : QObject(parent)
{
    connect(gmConn_, &GMConnection::gmNetworkResponse, this, &LocationGeocode::handleGMResponse);
    connect(censusConn_, &CensusGeocode::networkResponse, this, &LocationGeocode::handleCensusResponse);
    connect(arcGISConn_, &ARCGISGeocode::networkResponse, this, &LocationGeocode::handleArcGISResponse);

    connect(gmConn_, &GMConnection::statusMessage, this, &LocationGeocode::statusMessage);
    connect(gmConn_, &GMConnection::errorMessage, this, &LocationGeocode::errorMessage);
    connect(gmConn_, &GMConnection::debugMessage, this, &LocationGeocode::debugMessage);

    //connect(bridgeDB_, &BridgeDatabase::statusMessage, this, &LocationGeocode::statusMessage);
    connect(bridgeDB_, &BridgeDatabase::errorMessage, this, &LocationGeocode::errorMessage);
    connect(bridgeDB_, &BridgeDatabase::debugMessage, this, &LocationGeocode::debugMessage);
}

LocationGeocode::~LocationGeocode()
{

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
        arcGISConn_->geocodeLocation(key, locationsToGeocode_[key].toObject());
        //censusConn_->geocodeLocation(key, locationsToGeocode_[key].toObject());
        //gmConn_->geocodeLocation(key, locationsToGeocode_[key].toObject());
    }

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, QJsonObject());
        reset();
    }
}

void LocationGeocode::GeocodeUpdateLocations(const QString &key, const QList<QVariantMap> &argList)
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
        QString organizationKey = vMap["organization:key"].toString();
        mergeLocationsToGeocode(bridgeDB_->getLocationsToUpdate(organizationKey));
        mergeLocationsToGeocode(bridgeDB_->getGMLocationsWithBadGeocode(organizationKey));
    }

    qDebug() << "Size of locations to geocode is..." << locationsToGeocode_.size();
    for(auto key:locationsToGeocode_.keys())
    {
        activeJobs_.insert(key);
        arcGISConn_->geocodeLocation(key, locationsToGeocode_[key].toObject());
        //censusConn_->geocodeLocation(key, locationsToGeocode_[key].toObject());
        //gmConn_->geocodeLocation(key, locationsToGeocode_[key].toObject());
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

    //--------------------------
    QJsonObject jObj;

    if(response["status"].toString() == "OK")
    {
        jObj["geocodingQuality"]    = QJsonValue("AUTO");
        jObj["latitude"]            = response["results"].toArray().first()["geometry"]["location"]["lat"];
        jObj["longitude"]           = response["results"].toArray().first()["geometry"]["location"]["lng"];
        jObj["geocodingDate"]       = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }
    else
    {
        jObj["geocodingQuality"]    = QJsonValue("UNSUCCESSFULL");
    }
    geocodedLocations_[key] = jObj;
    //--------------------------

    qDebug() << activeJobs_.count();
    if(activeJobs_.empty())
    {
        emit finished(currentKey_, geocodedLocations_);
        reset();
    }
}

void LocationGeocode::handleCensusResponse(const QString &key, const QJsonValue &response)
{
    activeJobs_.remove(key);
    //-----------------------------
    QJsonObject jObj;
    QJsonObject resultObj = response["result"].toObject();
    QJsonArray addressMatches = resultObj["addressMatches"].toArray();

    if(addressMatches.isEmpty())
    {
        jObj["geocodingQuality"]    = QJsonValue("UNSUCCESSFULL");
    }
    else
    {
        jObj["geocodingQuality"]    =   QJsonValue("AUTO");
        jObj["latitude"]            =   addressMatches.first()["coordinates"]["y"];
        jObj["longitude"]           =   addressMatches.first()["coordinates"]["x"];
        jObj["geocodingDate"]       =   QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }
    geocodedLocations_[key] = jObj;
    //-----------------------------

    qDebug() << activeJobs_.count();
    if(activeJobs_.empty())
    {
        emit finished(currentKey_, geocodedLocations_);
        reset();
    }
}

void LocationGeocode::handleArcGISResponse(const QString &key, const QJsonValue &response)
{
    activeJobs_.remove(key);

    QJsonObject jObj;
    QJsonArray candidates = response.toObject()["candidates"].toArray();

    if(candidates.isEmpty())
    {
        jObj["geocodingQuality"]    = QJsonValue("UNSUCCESSFULL");
    }
    else
    {
        jObj["geocodingQuality"]    =   QJsonValue("AUTO");
        jObj["latitude"]            =   candidates.first()["location"]["y"];
        jObj["longitude"]           =   candidates.first()["location"]["x"];
        jObj["geocodingDate"]       =   QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }

    geocodedLocations_[key] = jObj;
    qDebug() << activeJobs_.count();
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
