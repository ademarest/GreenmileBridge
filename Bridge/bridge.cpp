#include "bridge.h"

Bridge::Bridge(QObject *parent) : QObject(parent)
{
    connect(queueTimer, &QTimer::timeout, this, &Bridge::processQueue);
    connect(dataCollector, &BridgeDataCollector::finished, this, &Bridge::handleJobCompletion);
    connect(locationGeocode_, &LocationGeocode::finished, this, &Bridge::handleJobCompletion);
    connect(locationUpload_, &LocationUpload::finished, this, &Bridge::handleJobCompletion);
    queueTimer->start(1000);
}


bool Bridge::hasActiveJobs()
{
    if(activeJobs_.isEmpty())
        return false;
    else
        return true;
}

void Bridge::addRequest(const QString &key)
{
    qDebug() << "START Bridge::addRequest.";

    QVariantMap request;

    for(auto jVal : settings_["daysToUpload"].toArray())
    {
        request["key"] = key;
        request["organization:key"] = settings_["organization:key"].toString();
        request["date"] = QDate::fromString(jVal.toString(), Qt::ISODate);
        request["scheduleTables"] = settings_["scheduleTables"].toArray();
        request["schedulePrimaryKeys"] = settings_["schedulePrimaryKeys"].toArray();
        requestQueue_.enqueue(request);
    }
    qDebug() << "END Bridge::addRequest.";
}

void Bridge::removeRequest(const QString &key)
{
    qDebug() << "START Bridge::removeRequest.";
    for(int i = 0; i < requestQueue_.size(); i++)
        if(requestQueue_[i]["key"] == key)
            requestQueue_.removeAt(i);
    qDebug() << "END Bridge::removeRequest.";
}

void Bridge::processQueue()
{
    if(hasActiveJobs() || requestQueue_.isEmpty())
        return;
    else
    {
        qDebug() << "START Bridge::processQueue.";
        currentRequest_ = requestQueue_.dequeue();
        QString jobKey = "collectData:" + currentRequest_["key"].toString();
        activeJobs_.insert(jobKey);
        startBridge(jobKey, currentRequest_["date"].toDate());
        qDebug() << "END Bridge::processQueue.";
    }
}

void Bridge::startBridge(const QString &key, const QDate &date)
{

    dataCollector->addRequest(key, date);
}

void Bridge::handleJobCompletion(const QString &key)
{
    qDebug() << settings_;
    QStringList splitKey = key.split(":");
    activeJobs_.remove(key);

    if(splitKey.first() == "collectData")
    {
        emit statusMessage(key + " has been completed.");
        applyScheduleHierarchy();
        generateArgs();

        QString jobKey = "geocodeLocations:" + currentRequest_["key"].toString();
        activeJobs_.insert(jobKey);
        locationGeocode_->GeocodeLocations(jobKey, argList_);
    }

    if(splitKey.first() == "geocodeLocations")
    {
        emit statusMessage(key + " has been completed.");

        QString jobKey = "uploadLocations:" + currentRequest_["key"].toString();
        activeJobs_.insert(jobKey);
        locationUpload_->UploadLocations(jobKey, argList_, locationGeocode_->getResults());
    }

    if(!hasActiveJobs())
    {
        emit finished(currentRequest_["key"].toString());
        qDebug() << "Finished job with key " << currentRequest_["key"].toString();
        currentRequest_.clear();
    }
}

void Bridge::applyScheduleHierarchy()
{
    QJsonArray scheduleTables = currentRequest_["scheduleTables"].toJsonArray();
    QJsonArray schedulePrimaryKeysJson = currentRequest_["schedulePrimaryKeys"].toJsonArray();
    QStringList schedulePrimaryKeys;
    for(auto jVal:schedulePrimaryKeysJson)
        schedulePrimaryKeys.append(jVal.toString());

    if(scheduleTables.size() > 1)
    {
        for(int i = 0; i < scheduleTables.size()-1; ++i)
        {
            qDebug() << "Enforcing table sanity.";
            qDebug() << "Primary table" << scheduleTables[i].toObject()["tableName"].toString();
            qDebug() << "Secondary table" << scheduleTables[i+1].toObject()["tableName"].toString();
            bridgeDB_->enforceTableSanity(schedulePrimaryKeys,
                                          scheduleTables[i].toObject()["tableName"].toString(),
                                          scheduleTables[i+1].toObject()["tableName"].toString());
        }
    }
}

void Bridge::generateArgs()
{
    argList_.clear();
    for(auto jVal:currentRequest_["scheduleTables"].toJsonArray())
    {
        QJsonObject tableObj = jVal.toObject();

        QVariantMap args;
        args["minRouteKey"] = tableObj["minRouteKey"].toString();
        args["maxRouteKey"] = tableObj["maxRouteKey"].toString();
        args["tableName"]   = tableObj["tableName"].toString();
        args["organization:key"] = currentRequest_["organization:key"].toString();
        args["date"] = currentRequest_["date"].toDate();
        argList_.append(args);
    }
    qDebug() << "Bridge::generateArgs result " << argList_;
}

//void Bridge::beginAnalysis(const QDate &date)
//{
//    QStringList assignmentTables = {"mrsDailyAssignments", "dlmrsDailyAssignments"};
//    QStringList pkList {"route:key", "route:date", "organization:key"};

//    bridgeDB->enforceTableSanity(pkList, "dlmrsDailyAssignments", "mrsDailyAssignments");
//    for(auto table:assignmentTables)
//    {
//        QString minDelim;
//        QString maxDelim;

//        if(table == "mrsDailyAssignments")
//        {
//            minDelim = "D";
//            maxDelim = "U";
//        }

//        uploadLocations(table, "SEATTLE", date, minDelim, maxDelim);
//        uploadRoutes(table, "SEATTLE", date, minDelim, maxDelim);
//        fixRouteAssignments(table, "SEATTLE", date, minDelim, maxDelim);
//    }

//    if(dataBucket_.isEmpty())
//    {
//        qDebug() << "bridge finished! nothing to do.";
//        emit finished();
//    }
//}

//void Bridge::changeToFinishedState()
//{
//    bridgeInProgress = false;
//    if(bridgeQueue.isEmpty())
//        return;
//    else
//        dataCollector->addRequest("another!", bridgeQueue.dequeue());
//}

//void Bridge::uploadLocations(  const QString &table,
//                               const QString &organizationKey,
//                               const QDate   &bridgeDate,
//                               const QString &minDelim,
//                               const QString &maxDelim)
//{
//    QJsonObject locationsToUpload = bridgeDB->getLocationsToUpload(table, organizationKey, bridgeDate, minDelim, maxDelim);
//    for(auto key:locationsToUpload.keys())
//    {
//        QString geocodeKey = "geocode:" + key;
//        dataBucket_[geocodeKey] = locationsToUpload[key].toObject();
//        gmConn->geocodeLocation(geocodeKey, dataBucket_[geocodeKey].toObject());
//    }
//}

//void Bridge::uploadRoutes(  const QString &table,
//                            const QString &organizationKey,
//                            const QDate   &bridgeDate,
//                            const QString &minDelim,
//                            const QString &maxDelim)
//{
//    QJsonObject routeUploadObj = bridgeDB->getRoutesToUpload(table, organizationKey, bridgeDate, minDelim, maxDelim);

//    for(auto key:routeUploadObj.keys())
//        dataBucket_[key] = routeUploadObj[key].toObject();

//    for(auto key:routeUploadObj.keys())
//    {
//        if(key.split(":").first() == "routeUpload")
//        {
//            gmConn->uploadARoute(key, dataBucket_[key].toObject());
//        }
//    }
//}

//void Bridge::fixRouteAssignments(const QString &table,
//                                 const QString &organizationKey,
//                                 const QDate   &bridgeDate,
//                                 const QString &minDelim,
//                                 const QString &maxDelim)
//{
//    QJsonObject reassignmentResultObj = bridgeDB->getAssignmentsToUpdate(table, organizationKey, bridgeDate, minDelim, maxDelim);
//    fixDriverAssignments(reassignmentResultObj);
//    fixEquipmentAssignments(reassignmentResultObj);
//}

//void Bridge::fixDriverAssignments(const QJsonObject &reassignmentResultObj)
//{
//    for(auto key:reassignmentResultObj.keys())
//    {
//        QStringList splitKey = key.split(":");

//        if(!splitKey.isEmpty())
//            splitKey.removeFirst();

//        QString routeDriverAssignmentKey = "routeDriverAssignment:" + splitKey.join(":");
//        QString routeDriverAssigmentDeletionKey = "routeDriverAssigmentDeletion:" + splitKey.join(":");

//        QJsonObject reassignmentObj = reassignmentResultObj[key].toObject();
//        QJsonObject routeDriverAssignmentObj;
//        QJsonObject routeEquipmentAssignmentObj;

//        if(reassignmentObj["driverAssignments:0:id"].type() == QJsonValue::Double)
//        {
//            qDebug() << "deleting" << reassignmentObj["driverAssignments:0:id"];
//            gmConn->deleteDriverAssignment(routeDriverAssigmentDeletionKey, reassignmentObj["driverAssignments:0:id"].toInt());
//        }

//        routeDriverAssignmentObj["route"] = QJsonObject{{"id", reassignmentObj["id"]}};
//        routeDriverAssignmentObj["driver"] = QJsonObject{{"id", reassignmentObj["driverAssignments:0:driver:id"]}};
//        qDebug() << "route driver reassignment obj" <<  routeDriverAssignmentObj;
//        dataBucket_[routeDriverAssignmentKey] = routeDriverAssignmentObj;
//        gmConn->assignDriverToRoute(routeDriverAssignmentKey, routeDriverAssignmentObj);
//    }
//}

//void Bridge::fixEquipmentAssignments(const QJsonObject &reassignmentResultObj)
//{
//    for(auto key:reassignmentResultObj.keys())
//    {
//        QStringList splitKey = key.split(":");

//        if(!splitKey.isEmpty())
//            splitKey.removeFirst();

//        QString routeEquipmentAssignmentKey = "routeEquipmentAssignment:" + splitKey.join(":");
//        QString routeEquipmentAssignmentDeletionKey = "routeEquipmentAssignmentDeletion:" + splitKey.join(":");

//        QJsonObject reassignmentObj = reassignmentResultObj[key].toObject();
//        QJsonObject routeEquipmentAssignmentObj;

//        if(reassignmentObj["equipmentAssignments:0:id"].type() == QJsonValue::Double)
//        {
//            qDebug() << "deleting" << reassignmentObj["equipmentAssignments:0:id"];
//            gmConn->deleteEquipmentAssignment(routeEquipmentAssignmentDeletionKey, reassignmentObj["equipmentAssignments:0:id"].toInt());
//        }

//        routeEquipmentAssignmentObj["route"]        = QJsonObject{{"id", reassignmentObj["id"]}};
//        routeEquipmentAssignmentObj["equipment"]    = QJsonObject{{"id", reassignmentObj["equipmentAssignments:0:equipment:id"]}};
//        routeEquipmentAssignmentObj["principal"]    = QJsonValue(true);
//        qDebug() << "route reassignment obj" <<  routeEquipmentAssignmentObj;
//        dataBucket_[routeEquipmentAssignmentKey] = routeEquipmentAssignmentObj;
//        gmConn->assignEquipmentToRoute(routeEquipmentAssignmentKey, routeEquipmentAssignmentObj);
//    }
//}

//void Bridge::handleGMResponse(const QString &key, const QJsonValue &val)
//{
//    qDebug() << "key just moved through handleGMResponse" << key;
//    if(key.split(":").first() == "geocode")
//    {
//        applyGeocodeResponseToLocation(key, val.toObject());

//        QStringList keyList  = key.split(":");
//        keyList[0] = "uploadLocation";
//        QString uploadLocationKey = keyList.join(":");
//        gmConn->uploadALocation(uploadLocationKey, dataBucket_[key].toObject());
//        handleJobCompletion(key);
//    }

//    if(key.split(":").first() == "uploadLocation")
//    {
//        qDebug() << key << "uploaded!";
//        handleJobCompletion(key);
//    }

//    if(key.split(":").first() == "routeUpload")
//    {

//        QJsonObject response = val.toObject();
//        QJsonObject routeDriverAssignmentObj;
//        QJsonObject routeEquipmentAssignmentObj;
//        QStringList keyList  = key.split(":");
//        keyList[0] = "routeDriverAssignment";
//        QString driverAssignmentKey = keyList.join(":");

//        QJsonObject routeAsnObj {{"id", response["id"]}};
//        routeDriverAssignmentObj["route"] = routeAsnObj;


//        dataBucket_[driverAssignmentKey];

//        routeDriverAssignmentObj["driver"] = dataBucket_[driverAssignmentKey];

//        gmConn->assignDriverToRoute(driverAssignmentKey, routeDriverAssignmentObj);
//        //---------------------------------------------------------------------
//        //Equipment time
//        //---------------------------------------------------------------------
//        keyList[0] = "routeEquipmentAssignment";
//        QString equipmentAssignmentKey = keyList.join(":");
//        QJsonObject equipmentAsnId {{"id", response["id"]}};
//        routeEquipmentAssignmentObj["route"] = equipmentAsnId;

//        qDebug() << dataBucket_.contains(equipmentAssignmentKey);
//        qDebug() << dataBucket_[equipmentAssignmentKey];
//        qDebug() << equipmentAssignmentKey;
//        routeEquipmentAssignmentObj["equipment"] = dataBucket_[equipmentAssignmentKey];
//        routeEquipmentAssignmentObj["principal"] = QJsonValue(true);

//        qDebug() << routeDriverAssignmentObj;
//        qDebug() << routeEquipmentAssignmentObj;
//        gmConn->assignEquipmentToRoute(equipmentAssignmentKey, routeEquipmentAssignmentObj);

//        handleJobCompletion(key);
//    }
//    if(key.split(":").first() == "routeDriverAssignment")
//    {
//        qDebug() << "routeDriverAssignment" <<  dataBucket_[key];
//        handleJobCompletion(key);
//    }
//    if(key.split(":").first() == "routeEquipmentAssignment")
//    {
//        qDebug() << "routeDriverAssignment" << dataBucket_[key];
//        handleJobCompletion(key);
//    }
//}

//void Bridge::applyGeocodeResponseToLocation(const QString &key, const QJsonObject &obj)
//{
//    QJsonObject dbObj = dataBucket_[key].toObject();

//    if(obj["status"].toString() == "OK")
//    {
//        dbObj["geocodingQuality"] = QJsonValue("AUTO");
//        dbObj["latitude"] = obj["results"].toArray().first()["geometry"]["location"]["lat"];
//        dbObj["longitude"] = obj["results"].toArray().first()["geometry"]["location"]["lng"];
//    }
//    else
//        dbObj["geocodingQuality"] = QJsonValue("UNSUCCESSFUL");

//    dataBucket_[key] = dbObj;
//    qDebug() << dataBucket_[key];
//}

//void Bridge::handleJobCompletion(const QString &key)
//{
//    dataBucket_.remove(key);
//    qDebug() << "job completion" <<  dataBucket_.keys() << key << dataBucket_.size();
//    if(!hasActiveJobs())
//    {
//        qDebug() << "bridge finished!";
//        emit finished();
//    }
//    else
//    {
//        qDebug() << "bucket keys" << dataBucket_.keys();
//    }
//}

