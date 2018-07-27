#include "locationupload.h"

LocationUpload::LocationUpload(QObject *parent) : QObject(parent)
{
    connect(gmConn_, &GMConnection::gmNetworkResponse, this, &LocationUpload::handleGMResponse);
}

//QJsonObject LocationUpload::getResults()
//{
//    return uploadedLocations_;
//}

void LocationUpload::reset()
{
    if(!activeJobs_.isEmpty())
    {
        errorMessage("Location upload in progress. Try again once current request is finished.");
        qDebug() << "Location upload in progress. Try again once current request is finished.";
        return;
    }

    currentKey_.clear();
    activeJobs_.clear();
    currentRequest_.clear();
    locationsToUpload_ = QJsonObject();
    uploadedLocations_ = QJsonObject();
}

void LocationUpload::UploadLocations(const QString &key, const QList<QVariantMap> &argList, const QJsonObject &geocodes)
{
    if(!activeJobs_.isEmpty())
    {
        errorMessage("Location upload in progress. Try again once current request is finished.");
        qDebug() << "Location upload in progress. Try again once current request is finished.";
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
        QString maxRouteKey = vMap["maxRouteKey"].toString();

        QJsonObject locations = bridgeDB_->getLocationsToUpload(tableName, organizationKey, date, minRouteKey, maxRouteKey);
        mergeLocationsToUpload(locations);
    }
    applyGeocodesToLocations(geocodes);

    for(auto key:locationsToUpload_.keys())
    {
        activeJobs_.insert(key);
        gmConn_->uploadALocation(key, locationsToUpload_[key].toObject());
    }

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, QJsonObject());
        reset();
    }
}

void LocationUpload::handleGMResponse(const QString &key, const QJsonValue &response)
{
    activeJobs_.remove(key);
    uploadedLocations_[key] = response;

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, uploadedLocations_);
        reset();
    }
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
    for(auto key:geocodes.keys())
    {

        QJsonObject jObj = locationsToUpload_[key].toObject();
        //qDebug() << "location" << key << locationsToUpload_[key].toObject();
        //qDebug() << "geocode" << key << geocodes[key];

        if(geocodes[key]["status"].toString() == "OK")
        {
            jObj["geocodingQuality"]    = QJsonValue("AUTO");
            jObj["latitude"]            = geocodes[key]["results"].toArray().first()["geometry"]["location"]["lat"];
            jObj["longitude"]           = geocodes[key]["results"].toArray().first()["geometry"]["location"]["lng"];
        }
        else
        {
            jObj["geocodingQuality"]    = QJsonValue("UNSUCCESSFULL");
        }
        locationsToUpload_[key] = jObj;
    }
}
