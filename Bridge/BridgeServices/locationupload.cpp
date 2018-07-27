#include "locationupload.h"

LocationUpload::LocationUpload(QObject *parent) : QObject(parent)
{
    connect(gmConn_, &GMConnection::gmNetworkResponse, this, &LocationUpload::handleGMResponse);
}

QJsonObject LocationUpload::getResults()
{
    return uploadedLocations_;
}

void LocationUpload::UploadLocations(const QString &key, const QList<QVariantMap> &argList, const QJsonObject &geocodes)
{
    if(!activeJobs_.isEmpty())
    {
        errorMessage("Geocoding in progress. Try again once current request is finished.");
        return;
    }

    currentKey_ = key;
    uploadedLocations_.empty();
    locationsToUpload_.empty();

    for(auto vMap:argList)
    {
        QString tableName = vMap["tableName"].toString();
        QString organizationKey = vMap["organization:key"].toString();
        QDate date = vMap["date"].toDate();
        QString minRouteKey = vMap["minRouteKey"].toString();
        QString maxRouteKey = vMap["minRouteKey"].toString();

        mergeLocationsToUpload(bridgeDB_->getLocationsToUpload(tableName, organizationKey, date, minRouteKey, maxRouteKey));
    }
    applyGeocodesToLocations(geocodes);

    for(auto key:locationsToUpload_.keys())
    {
        activeJobs_.insert(key);
        gmConn_->uploadALocation(key, locationsToUpload_[key].toObject());
    }
}

void LocationUpload::handleGMResponse(const QString &key, const QJsonValue &response)
{
    activeJobs_.remove(key);
    uploadedLocations_[key] = response;

    if(activeJobs_.empty())
        emit finished(currentKey_);
}

void LocationUpload::mergeLocationsToUpload(const QJsonObject &locations)
{
    for(auto key:locations.keys())
    {
        locationsToUpload_[key] = locations[key];
    }
}

void LocationUpload::applyGeocodesToLocations(const QJsonObject &geocodes)
{
    qDebug() << "geocode keys" << geocodes.keys();
    qDebug() << "upload keys" << locationsToUpload_.keys();

    for(auto key:geocodes.keys())
    {
        qDebug() << geocodes[key];
        QJsonObject jObj = locationsToUpload_[key].toObject();
        if(geocodes[key]["status"].toString() == "OK")
        {
            jObj["geocodingQuality"]    = QJsonValue("AUTO");
            jObj["latitude"]            = geocodes[key]["results"].toArray().first()["geometry"]["location"]["lat"];
            jObj["longitude"]           = geocodes[key]["results"].toArray().first()["geometry"]["location"]["lng"];
        }
        else
        {
            jObj["geocodingQuality"]    = QJsonValue("UNSUCCESSFUL");
        }

        locationsToUpload_[key] = jObj;
        qDebug() << "geocoded location" << locationsToUpload_[key].toObject();
    }
}


//void Bridge::applyGeocodeResponseToLocation(const QString &key, const QJsonObject &obj)
//{

//}
