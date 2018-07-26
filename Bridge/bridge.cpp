#include "bridge.h"

Bridge::Bridge(QObject *parent) : QObject(parent)
{
    connect(bridgeDB, &BridgeDatabase::statusMessage, this, &Bridge::statusMessage);
    connect(bridgeTimer, &QTimer::timeout, this, &Bridge::startBridge);
    connect(gmConn, &GMConnection::gmNetworkResponse, this, &Bridge::handleGMResponse);
    connect(gmConn, &GMConnection::statusMessage, this, &Bridge::statusMessage);
    connect(dataCollector, &BridgeDataCollector::finished, this, &Bridge::beginAnalysis);
    connect(this, &Bridge::finished, this, &Bridge::changeToFinishedState);
}


void Bridge::startBridge()
{
    bridgeTimer->start(600000);
    qDebug() << "Is bridge timer single shot? " <<  bridgeTimer->isSingleShot();

    bridgeQueue.enqueue(QDate::currentDate().addDays(1));

    dataCollector->addRequest("first", QDate::currentDate());
    //bridgeDate = QDate::currentDate().addDays(1);
}

void Bridge::beginAnalysis()
{
    QStringList assignmentTables = {"mrsDailyAssignments", "dlmrsDailyAssignments"};
    QStringList pkList {"route:key", "route:date", "organization:key"};

    bridgeDB->enforceTableSanity(pkList, "dlmrsDailyAssignments", "mrsDailyAssignments");
    for(auto table:assignmentTables)
    {
        QString minDelim;
        QString maxDelim;

        if(table == "mrsDailyAssignments")
        {
            minDelim = "D";
            maxDelim = "U";
        }

        uploadLocations(table, "SEATTLE", bridgeDate, minDelim, maxDelim);
        uploadRoutes(table, "SEATTLE", bridgeDate, minDelim, maxDelim);
        fixRouteAssignments(table, "SEATTLE", bridgeDate, minDelim, maxDelim);
    }

    if(dataBucket_.isEmpty())
    {
        qDebug() << "bridge finished! nothing to do.";
        emit finished();
    }
}

void Bridge::changeToFinishedState()
{
    bridgeInProgress = false;
    if(bridgeQueue.isEmpty())
        return;
    else
        dataCollector->addRequest("another!", bridgeQueue.dequeue());
}

void Bridge::uploadLocations(  const QString &table,
                               const QString &organizationKey,
                               const QDate   &bridgeDate,
                               const QString &minDelim,
                               const QString &maxDelim)
{
    QJsonObject locationsToUpload = bridgeDB->getLocationsToUpload(table, organizationKey, bridgeDate, minDelim, maxDelim);
    for(auto key:locationsToUpload.keys())
    {
        dataBucket_["geocode:" + key] = locationsToUpload[key].toObject();
        gmConn->geocodeLocation(key, dataBucket_["geocode:" + key].toObject());
    }
}

void Bridge::uploadRoutes(  const QString &table,
                            const QString &organizationKey,
                            const QDate   &bridgeDate,
                            const QString &minDelim,
                            const QString &maxDelim)
{
    QJsonObject routeUploadObj = bridgeDB->getRoutesToUpload(table, organizationKey, bridgeDate, minDelim, maxDelim);

    for(auto key:routeUploadObj.keys())
        dataBucket_[key] = routeUploadObj[key].toObject();

    for(auto key:routeUploadObj.keys())
    {
        if(key.split(":").first() == "routeUpload")
        {
            gmConn->uploadARoute(key, dataBucket_[key].toObject());
        }
    }
}

void Bridge::fixRouteAssignments(const QString &table,
                                 const QString &organizationKey,
                                 const QDate   &bridgeDate,
                                 const QString &minDelim,
                                 const QString &maxDelim)
{
    QJsonObject reassignmentResultObj = bridgeDB->getAssignmentsToUpdate(table, organizationKey, bridgeDate, minDelim, maxDelim);
    fixDriverAssignments(reassignmentResultObj);
    fixEquipmentAssignments(reassignmentResultObj);
}

void Bridge::fixDriverAssignments(const QJsonObject &reassignmentResultObj)
{
    for(auto key:reassignmentResultObj.keys())
    {
        QStringList splitKey = key.split(":");

        if(!splitKey.isEmpty())
            splitKey.removeFirst();

        QString routeDriverAssignmentKey = "routeDriverAssignment:" + splitKey.join(":");
        QString routeDriverAssigmentDeletionKey = "routeDriverAssigmentDeletion:" + splitKey.join(":");

        QJsonObject reassignmentObj = reassignmentResultObj[key].toObject();
        QJsonObject routeDriverAssignmentObj;
        QJsonObject routeEquipmentAssignmentObj;

        if(reassignmentObj["driverAssignments:0:id"].type() == QJsonValue::Double)
        {
            qDebug() << "deleting" << reassignmentObj["driverAssignments:0:id"];
            gmConn->deleteDriverAssignment(routeDriverAssigmentDeletionKey, reassignmentObj["driverAssignments:0:id"].toInt());
        }

        routeDriverAssignmentObj["route"] = QJsonObject{{"id", reassignmentObj["id"]}};
        routeDriverAssignmentObj["driver"] = QJsonObject{{"id", reassignmentObj["driverAssignments:0:driver:id"]}};
        qDebug() << "route driver reassignment obj" <<  routeDriverAssignmentObj;
        dataBucket_[routeDriverAssignmentKey] = routeDriverAssignmentObj;
        gmConn->assignDriverToRoute(routeDriverAssignmentKey, routeDriverAssignmentObj);
    }
}

void Bridge::fixEquipmentAssignments(const QJsonObject &reassignmentResultObj)
{
    for(auto key:reassignmentResultObj.keys())
    {
        QStringList splitKey = key.split(":");

        if(!splitKey.isEmpty())
            splitKey.removeFirst();

        QString routeEquipmentAssignmentKey = "routeEquipmentAssignment:" + splitKey.join(":");
        QString routeEquipmentAssignmentDeletionKey = "routeEquipmentAssignmentDeletion:" + splitKey.join(":");

        QJsonObject reassignmentObj = reassignmentResultObj[key].toObject();
        QJsonObject routeEquipmentAssignmentObj;

        if(reassignmentObj["equipmentAssignments:0:id"].type() == QJsonValue::Double)
        {
            qDebug() << "deleting" << reassignmentObj["equipmentAssignments:0:id"];
            gmConn->deleteEquipmentAssignment(routeEquipmentAssignmentDeletionKey, reassignmentObj["equipmentAssignments:0:id"].toInt());
        }

        routeEquipmentAssignmentObj["route"]        = QJsonObject{{"id", reassignmentObj["id"]}};
        routeEquipmentAssignmentObj["equipment"]    = QJsonObject{{"id", reassignmentObj["equipmentAssignments:0:equipment:id"]}};
        routeEquipmentAssignmentObj["principal"]    = QJsonValue(true);
        qDebug() << "route reassignment obj" <<  routeEquipmentAssignmentObj;
        dataBucket_[routeEquipmentAssignmentKey] = routeEquipmentAssignmentObj;
        gmConn->assignEquipmentToRoute(routeEquipmentAssignmentKey, routeEquipmentAssignmentObj);
    }
}

void Bridge::handleGMResponse(const QString &key, const QJsonValue &val)
{
    qDebug() << "key just moved through handleGMResponse" << key;
    if(key.split(":").first() == "geocode")
    {
        applyGeocodeResponseToLocation(key, val.toObject());

        QStringList keyList  = key.split(":");
        keyList[0] = "uploadLocation";
        QString uploadLocationKey = keyList.join(":");
        gmConn->uploadALocation(uploadLocationKey, dataBucket_[key].toObject());
        handleJobCompletion(key);
    }
    if(key.split(":").first() == "uploadLocation")
    {
        qDebug() << key << "uploaded!";
        handleJobCompletion(key);
    }
    if(key.split(":").first() == "routeUpload")
    {

        QJsonObject response = val.toObject();
        QJsonObject routeDriverAssignmentObj;
        QJsonObject routeEquipmentAssignmentObj;
        QStringList keyList  = key.split(":");
        keyList[0] = "routeDriverAssignment";
        QString driverAssignmentKey = keyList.join(":");

        QJsonObject routeAsnObj {{"id", response["id"]}};
        routeDriverAssignmentObj["route"] = routeAsnObj;


        dataBucket_[driverAssignmentKey];

        routeDriverAssignmentObj["driver"] = dataBucket_[driverAssignmentKey];

        gmConn->assignDriverToRoute(driverAssignmentKey, routeDriverAssignmentObj);
        //---------------------------------------------------------------------
        //Equipment time
        //---------------------------------------------------------------------
        keyList[0] = "routeEquipmentAssignment";
        QString equipmentAssignmentKey = keyList.join(":");
        QJsonObject equipmentAsnId {{"id", response["id"]}};
        routeEquipmentAssignmentObj["route"] = equipmentAsnId;

        qDebug() << dataBucket_.contains(equipmentAssignmentKey);
        qDebug() << dataBucket_[equipmentAssignmentKey];
        qDebug() << equipmentAssignmentKey;
        routeEquipmentAssignmentObj["equipment"] = dataBucket_[equipmentAssignmentKey];
        routeEquipmentAssignmentObj["principal"] = QJsonValue(true);

        qDebug() << routeDriverAssignmentObj;
        qDebug() << routeEquipmentAssignmentObj;
        gmConn->assignEquipmentToRoute(equipmentAssignmentKey, routeEquipmentAssignmentObj);

        handleJobCompletion(key);
    }
    if(key.split(":").first() == "routeDriverAssignment")
    {
        qDebug() << "routeDriverAssignment" <<  dataBucket_[key];
        handleJobCompletion(key);
    }
    if(key.split(":").first() == "routeEquipmentAssignment")
    {
        qDebug() << "routeDriverAssignment" << dataBucket_[key];
        handleJobCompletion(key);
    }
}

void Bridge::applyGeocodeResponseToLocation(const QString &key, const QJsonObject &obj)
{
    QJsonObject dbObj = dataBucket_[key].toObject();

    if(obj["status"].toString() == "OK")
    {
        dbObj["geocodingQuality"] = QJsonValue("AUTO");
        dbObj["latitude"] = obj["results"].toArray().first()["geometry"]["location"]["lat"];
        dbObj["longitude"] = obj["results"].toArray().first()["geometry"]["location"]["lng"];
    }
    else
        dbObj["geocodingQuality"] = QJsonValue("UNSUCCESSFUL");

    dataBucket_[key] = dbObj;
    qDebug() << dataBucket_[key];
}

void Bridge::handleJobCompletion(const QString &key)
{
    dataBucket_.remove(key);
    qDebug() << "job completion" <<  dataBucket_.keys() << key << dataBucket_.size();
    if(!hasActiveJobs())
    {
        qDebug() << "bridge finished!";
        emit finished();
    }
    else
    {
        qDebug() << "bucket keys" << dataBucket_.keys();
    }
}

bool Bridge::hasActiveJobs()
{
    if(dataBucket_.isEmpty())
        return false;
    else
        return true;
}
