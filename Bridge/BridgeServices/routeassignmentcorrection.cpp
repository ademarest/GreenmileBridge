#include "routeassignmentcorrection.h"

RouteAssignmentCorrection::RouteAssignmentCorrection(QObject *parent) : QObject(parent)
{
    connect(gmDeleteConn_, &GMConnection::gmNetworkResponse, this, &RouteAssignmentCorrection::handleGMDeleteAssignmentResponse);
    connect(gmAssignConn_, &GMConnection::gmNetworkResponse, this, &RouteAssignmentCorrection::handleGMUploadAssignmentResponse);

    connect(gmDeleteConn_, &GMConnection::statusMessage, this, &RouteAssignmentCorrection::statusMessage);
    connect(gmDeleteConn_, &GMConnection::errorMessage, this, &RouteAssignmentCorrection::errorMessage);
    connect(gmDeleteConn_, &GMConnection::debugMessage, this, &RouteAssignmentCorrection::debugMessage);

    connect(gmAssignConn_, &GMConnection::statusMessage, this, &RouteAssignmentCorrection::statusMessage);
    connect(gmAssignConn_, &GMConnection::errorMessage, this, &RouteAssignmentCorrection::errorMessage);
    connect(gmAssignConn_, &GMConnection::debugMessage, this, &RouteAssignmentCorrection::debugMessage);

    //connect(bridgeDB_, &BridgeDatabase::statusMessage, this, &RouteAssignmentCorrection::statusMessage);
    connect(bridgeDB_, &BridgeDatabase::errorMessage, this, &RouteAssignmentCorrection::errorMessage);
    connect(bridgeDB_, &BridgeDatabase::debugMessage, this, &RouteAssignmentCorrection::debugMessage);
}


void RouteAssignmentCorrection::reset()
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
    routeAssignmentsToCorrect_ = QJsonObject();
    correctedRouteAssignments_ = QJsonObject();
}

void RouteAssignmentCorrection::CorrectRouteAssignments(const QString &key, const QList<QVariantMap> &argList)
{

    if(!activeJobs_.isEmpty())
    {
        errorMessage("Geocoding in progress. Try again once current request is finished.");
        qDebug() << "Geocoding in progress. Try again once current request is finished.";
        return;
    }

    currentKey_ = key;
    correctedRouteAssignments_.empty();
    routeAssignmentsToCorrect_.empty();

    for(auto vMap:argList)
    {
        QString tableName = vMap["tableName"].toString();
        QString organizationKey = vMap["organization:key"].toString();
        QDate date = vMap["date"].toDate();
        QString minRouteKey = vMap["minRouteKey"].toString();
        QString maxRouteKey = vMap["maxRouteKey"].toString();

        mergeRouteAssignmentCorrections(bridgeDB_->getAssignmentsToUpdate(tableName, organizationKey, date, minRouteKey, maxRouteKey));
    }

    if(routeAssignmentsToCorrect_.empty())
    {
        emit finished(currentKey_, QJsonObject());
        reset();
    }
    else
    {
        deleteAssignments();
    }
}

void RouteAssignmentCorrection::mergeRouteAssignmentCorrections(const QJsonObject &locations)
{
    for(auto key:locations.keys())
    {
        routeAssignmentsToCorrect_[key] = locations[key];
    }
}

void RouteAssignmentCorrection::deleteAssignments()
{
    int deleteRequestCount = 0;
    for(auto key:routeAssignmentsToCorrect_.keys())
    {
        QStringList splitKey = key.split(":");

        if(!splitKey.isEmpty())
            splitKey.removeFirst();

        QString routeEquipmentAssignmentKey = "routeEquipmentAssignment:" + splitKey.join(":");
        QString routeEquipmentAssignmentDeletionKey = "routeEquipmentAssignmentDeletion:" + splitKey.join(":");

        QJsonObject reassignmentObj = routeAssignmentsToCorrect_[key].toObject();
        QJsonObject routeEquipmentAssignmentObj;

        if(reassignmentObj["equipmentAssignments:0:id"].type() == QJsonValue::Double)
        {
            qDebug() << "deleting int " << reassignmentObj["equipmentAssignments:0:id"].toInt() << "double" << reassignmentObj["equipmentAssignments:0:id"].toDouble() << reassignmentObj["equipmentAssignments:0:id"].toString();
            QString entityID = QString::number(reassignmentObj["driverAssignments:0:id"].toInt());
            qDebug() << entityID;
            activeJobs_.insert(routeEquipmentAssignmentDeletionKey);
            gmDeleteConn_->deleteEquipmentAssignment(routeEquipmentAssignmentDeletionKey, entityID);
            deleteRequestCount++;
        }
    }

    for(auto key:routeAssignmentsToCorrect_.keys())
    {
        QStringList splitKey = key.split(":");

        if(!splitKey.isEmpty())
            splitKey.removeFirst();

        QString routeDriverAssignmentKey = "routeDriverAssignment:" + splitKey.join(":");
        QString routeDriverAssigmentDeletionKey = "routeDriverAssigmentDeletion:" + splitKey.join(":");

        QJsonObject reassignmentObj = routeAssignmentsToCorrect_[key].toObject();
        QJsonObject routeDriverAssignmentObj;
        QJsonObject routeEquipmentAssignmentObj;

        if(reassignmentObj["driverAssignments:0:id"].type() == QJsonValue::Double)
        {
            qDebug() << "deleting int " << reassignmentObj["driverAssignments:0:id"].toInt() << "double" << reassignmentObj["driverAssignments:0:id"].toDouble() << reassignmentObj["driverAssignments:0:id"].toString();
            QString entityID = QString::number(reassignmentObj["driverAssignments:0:id"].toInt());
            qDebug() << "entityID before arg" << entityID;
            activeJobs_.insert(routeDriverAssigmentDeletionKey);
            gmDeleteConn_->deleteDriverAssignment(routeDriverAssigmentDeletionKey, entityID);
            deleteRequestCount++;
        }
    }

    if(deleteRequestCount == 0)
    {
        qDebug() << "straight to upload assignments";
        uploadAssignments();
    }
}

void RouteAssignmentCorrection::uploadAssignments()
{
    for(auto key:routeAssignmentsToCorrect_.keys())
    {
        QStringList splitKey = key.split(":");

        if(!splitKey.isEmpty())
            splitKey.removeFirst();

        QString routeEquipmentAssignmentKey = "routeEquipmentAssignment:" + splitKey.join(":");
        QJsonObject reassignmentObj = routeAssignmentsToCorrect_[key].toObject();
        QJsonObject routeEquipmentAssignmentObj;

        routeEquipmentAssignmentObj["route"]        = QJsonObject{{"id", reassignmentObj["id"]}};
        routeEquipmentAssignmentObj["equipment"]    = QJsonObject{{"id", reassignmentObj["equipmentAssignments:0:equipment:id"]}};
        routeEquipmentAssignmentObj["principal"]    = QJsonValue(true);
        qDebug() << "route equipment reassignment obj" << routeEquipmentAssignmentKey <<  routeEquipmentAssignmentObj;

        activeJobs_.insert(routeEquipmentAssignmentKey);
        gmAssignConn_->assignEquipmentToRoute(routeEquipmentAssignmentKey, routeEquipmentAssignmentObj);
    }

    for(auto key:routeAssignmentsToCorrect_.keys())
    {
        QStringList splitKey = key.split(":");

        if(!splitKey.isEmpty())
            splitKey.removeFirst();

        QString routeDriverAssignmentKey = "routeDriverAssignment:" + splitKey.join(":");
        QJsonObject reassignmentObj = routeAssignmentsToCorrect_[key].toObject();
        QJsonObject routeDriverAssignmentObj;

        routeDriverAssignmentObj["route"] = QJsonObject{{"id", reassignmentObj["id"]}};
        routeDriverAssignmentObj["driver"] = QJsonObject{{"id", reassignmentObj["driverAssignments:0:driver:id"]}};
        qDebug() << "route driver reassignment obj" << routeDriverAssignmentKey << routeDriverAssignmentObj;

        activeJobs_.insert(routeDriverAssignmentKey);
        gmAssignConn_->assignDriverToRoute(routeDriverAssignmentKey, routeDriverAssignmentObj);
    }
}

void RouteAssignmentCorrection::handleGMDeleteAssignmentResponse(const QString &key, const QJsonValue &response)
{
    activeJobs_.remove(key);
    correctedRouteAssignments_[key] = response;
    qDebug() << "active delete jobs " << activeJobs_;

    if(activeJobs_.empty())
    {
        qDebug() << "upload assignments";
        uploadAssignments();
    }
}

void RouteAssignmentCorrection::handleGMUploadAssignmentResponse(const QString &key, const QJsonValue &response)
{
    activeJobs_.remove(key);
    correctedRouteAssignments_[key] = response;
    qDebug() << "active assignment jobs " << activeJobs_;

    if(activeJobs_.empty())
    {
        qDebug() << "finished assignments";
        emit finished(currentKey_, correctedRouteAssignments_);
        reset();
    }
}
