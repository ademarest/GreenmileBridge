#include "bridge.h"

Bridge::Bridge(QObject *parent) : QObject(parent)
{
    connect(queueTimer, &QTimer::timeout, this, &Bridge::processQueue);
    connect(bridgeTimer, &QTimer::timeout, this, &Bridge::startOnTimer);
    connect(bridgeMalfunctionTimer, &QTimer::timeout, this, &Bridge::abort);
    init();
}

void Bridge::init()
{

    /*  Location PUT query.
    SELECT `location:enabled`, `location:key`, `location:description`, `location:addressLine1`, `location:addressLine2`, `location:city`, `location:state`, `location:zipCode`, `location:deliveryDays`, `id` FROM as400LocationQuery LEFT JOIN gmLocations ON `key` = `location:key` WHERE gmLocations.`key` IN ( SELECT `location:key` FROM ( SELECT DISTINCT `location:enabled`, `location:key`, `location:description`, `location:addressLine1`, `location:addressLine2`, `location:city`, `location:state`, `location:zipCode`, `location:deliveryDays` FROM as400LocationQuery WHERE as400LocationQuery.`organization:key` = 'SEATTLE' EXCEPT SELECT DISTINCT `enabled`, `key`,`description`,`addressLine1`,`addressLine2`,`city`, `state`,`zipCode`,`deliveryDays` FROM gmLocations WHERE gmLocations.`organization:key` = 'SEATTLE' ) )
    */

    qDebug() << settings_;

    connect(dataCollector, &BridgeDataCollector::finished, this, &Bridge::finishedDataCollection);

    connect(locationUpdateGeocode_, &LocationGeocode::finished, this, &Bridge::finishedLocationUpdateGeocode);
    connect(locationUpdate_, &LocationUpload::finished, this, &Bridge::finishedLocationUpdate);

    connect(locationUploadGeocode_, &LocationGeocode::finished, this, &Bridge::finishedLocationUploadGeocode);
    connect(locationUpload_, &LocationUpload::finished, this, &Bridge::finishedLocationUpload);

    connect(routeCheck_, &RouteCheck::finished, this, &Bridge::finishedRouteCheck);
    connect(routeUpload_, &RouteUpload::finished, this, &Bridge::finishedRouteUpload);
    connect(routeAssignmentCorrection_, &RouteAssignmentCorrection::finished, this, &Bridge::finishedRouteAssignmentCorrections);

    connect(dataCollector, &BridgeDataCollector::progress, this, &Bridge::currentJobProgress);
    connect(dataCollector, &BridgeDataCollector::statusMessage, this, &Bridge::statusMessage);
    connect(dataCollector, &BridgeDataCollector::errorMessage, this, &Bridge::errorMessage);

    connect(locationUploadGeocode_, &LocationGeocode::statusMessage, this, &Bridge::statusMessage);
    connect(locationUploadGeocode_, &LocationGeocode::errorMessage, this, &Bridge::errorMessage);

    connect(locationUpload_, &LocationUpload::statusMessage, this, &Bridge::statusMessage);
    connect(locationUpload_, &LocationUpload::errorMessage, this, &Bridge::errorMessage);

    connect(routeCheck_, &RouteCheck::statusMessage, this, &Bridge::statusMessage);
    connect(routeCheck_, &RouteCheck::errorMessage, this, &Bridge::errorMessage);

    connect(routeUpload_, &RouteUpload::statusMessage, this, &Bridge::statusMessage);
    connect(routeUpload_, &RouteUpload::errorMessage, this, &Bridge::errorMessage);

    connect(routeAssignmentCorrection_, &RouteAssignmentCorrection::statusMessage, this, &Bridge::statusMessage);
    connect(routeAssignmentCorrection_, &RouteAssignmentCorrection::errorMessage, this, &Bridge::errorMessage);

    bridgeTimer->start(600000);
    queueTimer->start(1000);
}

void Bridge::startOnTimer()
{
    addRequest("AUTO_START_TIMER");
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
    QVariantMap request;

    settings_ = QJsonObject{{"daysToUpload", QJsonValue(QJsonArray{QDate::currentDate().toString(Qt::ISODate), QDate::currentDate().addDays(1).toString(Qt::ISODate)})},
                            {"scheduleTables", QJsonValue(QJsonArray{QJsonValue(QJsonObject{{"tableName", QJsonValue("dlmrsDailyAssignments")}}),
                                                                     QJsonValue(QJsonObject{{"tableName", QJsonValue("mrsDailyAssignments")}, {"minRouteKey", "D"}, {"maxRouteKey", "U"}})})},
                            {"organization:key", QJsonValue("SEATTLE")},
                            {"monthsUntilCustDisabled", QJsonValue(3)},
                            {"schedulePrimaryKeys", QJsonValue(QJsonArray{"route:key", "route:date", "organization:key"})}};

    for(auto jVal : settings_["daysToUpload"].toArray())
    {
        request["key"] = key;
        request["organization:key"] = settings_["organization:key"].toString();
        request["date"] = QDate::fromString(jVal.toString(), Qt::ISODate);
        request["scheduleTables"] = settings_["scheduleTables"].toArray();
        request["schedulePrimaryKeys"] = settings_["schedulePrimaryKeys"].toArray();
        request["monthsUntilCustDisabled"] = settings_["monthsUntilCustDisabled"].toInt();
        requestQueue_.enqueue(request);
    }
}

void Bridge::removeRequest(const QString &key)
{
    for(int i = 0; i < requestQueue_.size(); i++)
        if(requestQueue_[i]["key"] == key)
            requestQueue_.removeAt(i);
}

void Bridge::processQueue()
{
    if(hasActiveJobs() || requestQueue_.isEmpty())
        return;
    else
    {
        currentRequest_ = requestQueue_.dequeue();

        emit started(currentRequest_["key"].toString());
        QString jobKey = "initialCollection:" + currentRequest_["key"].toString();

        emit statusMessage("-------------------------------------------------");
        emit statusMessage("Bridge started for: " + currentRequest_["key"].toString() + "_" + currentRequest_["date"].toDate().toString(Qt::ISODate) + ".");
        emit statusMessage("-------------------------------------------------");

        emit bridgeKeyChanged(currentRequest_["key"].toString() + "_" + currentRequest_["date"].toDate().toString(Qt::ISODate));
        addActiveJob(jobKey);
        startDataCollection(jobKey, currentRequest_["date"].toDate(), currentRequest_["monthsUntilCustDisabled"].toInt());

        bridgeMalfunctionTimer->start(4500000);
    }
}

void Bridge::addActiveJob(const QString &key)
{
    //addActiveJob(jobKey);
    emit currentJobChanged(key);
    activeJobs_.insert(key);

    ++totalJobCount_;
    activeJobCount_ = activeJobs_.size();

    emit bridgeProgress(activeJobCount_, totalJobCount_);
}

void Bridge::removeActiveJob(const QString &key)
{
    activeJobs_.remove(key);
    activeJobCount_ = activeJobs_.size();

    emit bridgeProgress(activeJobCount_, totalJobCount_);
}

void Bridge::startDataCollection(const QString &key, const QDate &date, const int monthsUntilCustDisabled)
{
    emit currentJobChanged(key);
    dataCollector->addRequest(key, date, monthsUntilCustDisabled);
}

void Bridge::finishedDataCollection(const QString &key)
{
    emit statusMessage(key + " has been completed.");

    applyScheduleHierarchy();
    generateArgs();

    if(key.split(":").first() == "initialCollection")
    {
        bridgeDB_->getLocationsToUpdate("SEATTLE");
        qDebug() << "geocoding updated locations";
        QString jobKey = "geocodeUpdatedLocations:" + currentRequest_["key"].toString();
        addActiveJob(jobKey);
        locationUpdateGeocode_->GeocodeUpdateLocations(jobKey, argList_);
    }

    if(key.split(":").first() == "refreshDataForRouteAssignmentCorrections")
    {
        qDebug() << "correcting route assignments";
        QString jobKey = "routeAssignmentCorrections:" + currentRequest_["key"].toString();
        addActiveJob(jobKey);
        routeAssignmentCorrection_->CorrectRouteAssignments(jobKey, argList_);
    }

    handleJobCompletion(key);
}

void Bridge::finishedLocationUpdateGeocode(const QString &key, const QJsonObject &result)
{
    emit statusMessage(key + " has been completed.");

    QString jobKey = "updateLocations:" + currentRequest_["key"].toString();
    addActiveJob(jobKey);
    locationUpload_->UpdateLocations(jobKey, argList_, result);
    handleJobCompletion(key);
}

void Bridge::finishedLocationUpdate(const QString &key, const QJsonObject &result)
{
    emit statusMessage(key + " has been completed.");
    qDebug() << result;

    QString jobKey = "geocodeUploadLocations:" + currentRequest_["key"].toString();
    addActiveJob(jobKey);
    locationUploadGeocode_->GeocodeLocations(jobKey, argList_);
    handleJobCompletion(key);
}

void Bridge::finishedLocationUploadGeocode(const QString &key, const QJsonObject &result)
{
    emit statusMessage(key + " has been completed.");

    QString jobKey = "uploadLocations:" + currentRequest_["key"].toString();
    addActiveJob(jobKey);
    locationUpload_->UploadLocations(jobKey, argList_, result);
    handleJobCompletion(key);
}

void Bridge::finishedLocationUpload(const QString &key, const QJsonObject &result)
{
    emit statusMessage(key + " has been completed.");
    qDebug() << result;

    QString jobKey = "routeCheck:" + currentRequest_["key"].toString();
    addActiveJob(jobKey);
    routeCheck_->deleteIncorrectRoutes(jobKey, argList_);
    handleJobCompletion(key);
}

void Bridge::finishedRouteCheck(const QString &key, const QJsonObject &result)
{
    emit statusMessage(key + " has been completed.");
    qDebug() << result;

    QString jobKey = "uploadRoutes:" + currentRequest_["key"].toString();
    addActiveJob(jobKey);
    routeUpload_->UploadRoutes(jobKey, argList_);
    handleJobCompletion(key);
}

void Bridge::finishedRouteUpload(const QString &key, const QJsonObject &result)
{
    emit statusMessage(key + " has been completed.");
    qDebug() << result;

    QString jobKey = "refreshDataForRouteAssignmentCorrections:" + currentRequest_["key"].toString();
    addActiveJob(jobKey);
    dataCollector->addRequest(jobKey,
                            currentRequest_["date"].toDate(),
                            currentRequest_["monthsUntilCustDisabled"].toInt(),
                            QStringList{"gmRoutes"});

    handleJobCompletion(key);
}

void Bridge::finishedRouteAssignmentCorrections(const QString &key, const QJsonObject &result)
{
    emit statusMessage(key + " has been completed.");
    qDebug() << result;
    handleJobCompletion(key);
}

void Bridge::handleJobCompletion(const QString &key)
{
    qDebug() << activeJobs_.size() << "jobs remaining" << activeJobs_ << key;
    emit statusMessage("The remaining bridge jobs are: " + activeJobs_.toList().join(",") + ".");
    removeActiveJob(key);
    if(!hasActiveJobs())
    {
        currentJobChanged(QString());
        activeJobCount_ = 0;
        totalJobCount_ = 0;

        bridgeProgress(activeJobCount_, totalJobCount_);

        bridgeMalfunctionTimer->stop();
        emit finished(currentRequest_["key"].toString());
        emit currentJobChanged("Done");

        qDebug() << "Finished job with key " << currentRequest_["key"].toString();
        emit statusMessage("-------------------------------------------------");
        emit statusMessage("Bridge finished for: " + currentRequest_["key"].toString() + "_" + currentRequest_["date"].toDate().toString(Qt::ISODate) + ".");
        emit statusMessage("-------------------------------------------------");

        currentRequest_.clear();
    }
}

void Bridge::abort()
{
    QString key = currentRequest_["key"].toString();
    emit errorMessage("ERROR: " + key + " ABORTED.");

    queueTimer->stop();
    bridgeTimer->stop();
    bridgeMalfunctionTimer->stop();

    requestQueue_.clear();
    argList_.clear();
    currentRequest_.clear();
    activeJobs_.clear();

    dataCollector->deleteLater();
    bridgeDB_->deleteLater();
    locationUploadGeocode_->deleteLater();
    locationUpload_->deleteLater();
    locationUpdateGeocode_->deleteLater();
    locationUpdate_->deleteLater();
    routeCheck_->deleteLater();
    routeUpload_->deleteLater();
    routeAssignmentCorrection_->deleteLater();

    totalJobCount_ = 0;
    activeJobCount_ = 0;
    emit bridgeKeyChanged("ABORTED");
    emit currentJobChanged("ABORTED");
    emit bridgeProgress(0, 0);
    emit currentJobProgress(0, 0);
    emit aborted(key);
    rebuild(key);
}

void Bridge::rebuild(const QString &key)
{
    dataCollector               = new BridgeDataCollector(this);
    bridgeDB_                   = new BridgeDatabase(this);
    locationUploadGeocode_      = new LocationGeocode(this);
    locationUpload_             = new LocationUpload(this);
    locationUpdateGeocode_      = new LocationGeocode(this);
    locationUpdate_             = new LocationUpload(this);
    routeCheck_                 = new RouteCheck(this);
    routeUpload_                = new RouteUpload(this);
    routeAssignmentCorrection_  = new RouteAssignmentCorrection(this);

    init();

    emit statusMessage("Bridge has been reset. Restarting queue.");
    emit rebuilt(key);
    bridgeTimer->start(600000);
    queueTimer->start(1000);
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
        args["key"]         = currentRequest_["key"].toString();
        args["minRouteKey"] = tableObj["minRouteKey"].toString();
        args["maxRouteKey"] = tableObj["maxRouteKey"].toString();
        args["tableName"]   = tableObj["tableName"].toString();
        args["organization:key"] = currentRequest_["organization:key"].toString();
        args["date"] = currentRequest_["date"].toDate();
        argList_.append(args);
    }
}


