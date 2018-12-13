#include "locationgeocode.h"

LocationGeocode::LocationGeocode(QObject *parent) : QObject(parent)
{
    connect(gmConn_, &GMConnection::networkResponse, this, &LocationGeocode::handleGMResponse);
    connect(censusConn_, &CensusGeocode::networkResponse, this, &LocationGeocode::handleCensusResponse);
    connect(arcGISConn_, &ARCGISGeocode::networkResponse, this, &LocationGeocode::handleArcGISResponse);

    connect(gmConn_, &GMConnection::statusMessage,  this, &LocationGeocode::statusMessage);
    connect(gmConn_, &GMConnection::errorMessage,   this, &LocationGeocode::errorMessage);
    connect(gmConn_, &GMConnection::debugMessage,   this, &LocationGeocode::debugMessage);
    connect(gmConn_, &GMConnection::failed,         this, &LocationGeocode::acknowledgeFailure);

    //connect(bridgeDB_, &BridgeDatabase::statusMessage, this, &LocationGeocode::statusMessage);
    connect(bridgeDB_, &BridgeDatabase::errorMessage,   this, &LocationGeocode::errorMessage);
    connect(bridgeDB_, &BridgeDatabase::statusMessage,  this, &LocationGeocode::statusMessage);
    connect(bridgeDB_, &BridgeDatabase::debugMessage,   this, &LocationGeocode::debugMessage);
    connect(bridgeDB_, &BridgeDatabase::failed,         this, &LocationGeocode::acknowledgeFailure);

    connect(arcGISConn_, &ARCGISGeocode::errorMessage,  this, &LocationGeocode::errorMessage);
    connect(arcGISConn_, &ARCGISGeocode::debugMessage,  this, &LocationGeocode::debugMessage);
    connect(arcGISConn_, &ARCGISGeocode::statusMessage, this, &LocationGeocode::statusMessage);
    connect(arcGISConn_, &ARCGISGeocode::failed,        this, &LocationGeocode::acknowledgeFailure);
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

    failState_ = false;
    currentKey_.clear();
    activeJobs_.clear();
    currentRequest_.clear();
    locationsToGeocode_ = QJsonObject();
    geocodedLocations_ = QJsonObject();
}

void LocationGeocode::GeocodeLocations(const QString &key, const QList<QVariantMap> &argList, const bool update, const bool fixBadGeocodes)
{
    QString geocodingService = "arcgis";
    QString jobKey = "GeocodeUpdateLocations";

    if(!activeJobs_.isEmpty())
    {
        errorMessage("Geocoding in progress. Try again once current request is finished.");
        qDebug() << "Geocoding in progress. Try again once current request is finished.";
        return;
    }

    currentKey_ = key;
    geocodedLocations_.empty();
    locationsToGeocode_.empty();

    getLocationsToGeocode(argList, update, fixBadGeocodes);
    qDebug() << "Size of locations to geocode is..." << locationsToGeocode_.size();
    startGeocoding(geocodingService);

    handleJobCompletion(jobKey);
}

void LocationGeocode::getLocationsToGeocode(QList<QVariantMap> argList, const bool update, const bool fixBadGeocodes)
{
    for(auto vMap:argList)
    {
        if(failState_)
            return;

        QString tableName = vMap["tableName"].toString();
        QString organizationKey = vMap["organization:key"].toString();
        QDate date = vMap["date"].toDate();
        QString minRouteKey = vMap["minRouteKey"].toString();
        QString maxRouteKey = vMap["maxRouteKey"].toString();

        if(update)
        {
            mergeLocationsToGeocode(bridgeDB_->getLocationsToUpdateGeocodes(organizationKey));
        }
        else
        {
            mergeLocationsToGeocode(bridgeDB_->getLocationsToUpload(tableName, organizationKey, date, minRouteKey, maxRouteKey));
        }
        if(fixBadGeocodes)
        {
            mergeLocationsToGeocode(bridgeDB_->getGMLocationsWithBadGeocode(organizationKey));
        }
    }
}

void LocationGeocode::startGeocoding(const QString &geocodingService)
{
    if(failState_)
        return;

    if(geocodingService == "arcgis")
    {
        dispatchGeocodeRequests(arcGISConn_);
    }
    if(geocodingService == "greenmile")
    {
        dispatchGeocodeRequests(gmConn_);
    }
    if(geocodingService == "census")
    {
        dispatchGeocodeRequests(censusConn_);
    }
}


void LocationGeocode::handleGMResponse(const QString &key, const QJsonValue &response)
{
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
    handleJobCompletion(key);
}

void LocationGeocode::handleCensusResponse(const QString &key, const QJsonValue &response)
{
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
    handleJobCompletion(key);
}

void LocationGeocode::handleArcGISResponse(const QString &key, const QJsonValue &response)
{
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
    handleJobCompletion(key);
}

void LocationGeocode::acknowledgeFailure(const QString &key, const QString &reason)
{
    failKey_    = key;
    failReason_ = reason;
    failState_  = true;
}


void LocationGeocode::handleJobCompletion(const QString &key)
{
    activeJobs_.remove(key);

    if(failState_)
    {
        emit failed("LocationGeocode::handleJobCompletion " + failKey_, failReason_);
        reset();
        return;
    }

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
