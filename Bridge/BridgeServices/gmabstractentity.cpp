#include "gmabstractentity.h"

GMAbstractEntity::GMAbstractEntity(QObject *parent) : QObject(parent)
{
    connect(gmConn_,    &GMConnection::networkResponse,   this, &GMAbstractEntity::handleGMResponse);

    connect(gmConn_,    &GMConnection::statusMessage,       this, &GMAbstractEntity::statusMessage);
    connect(gmConn_,    &GMConnection::errorMessage,        this, &GMAbstractEntity::errorMessage);
    connect(gmConn_,    &GMConnection::debugMessage,        this, &GMAbstractEntity::debugMessage);
    connect(gmConn_,    &GMConnection::failed,              this, &GMAbstractEntity::handleFailure);

    connect(bridgeDB_, &BridgeDatabase::errorMessage,       this, &GMAbstractEntity::errorMessage);
    connect(bridgeDB_, &BridgeDatabase::statusMessage,      this, &GMAbstractEntity::statusMessage);
    connect(bridgeDB_, &BridgeDatabase::debugMessage,       this, &GMAbstractEntity::debugMessage);
    connect(bridgeDB_, &BridgeDatabase::failed,             this, &GMAbstractEntity::handleFailure);
}

GMAbstractEntity::~GMAbstractEntity()
{

}

void GMAbstractEntity::handleGMResponse(const QString &key, QJsonValue response)
{
    activeJobs_.remove(key);
    QStringList keyList = key.split(":");
    if(keyList.isEmpty())
    {
        emit errorMessage("GMAbstractEntity::handleGMResponse key was empty.");
        return;
    }

    if(keyList.first() == "upload" && modFuncs_.contains("upload"))
    {
        entitiesUploaded_[key] = modFuncs_["upload"](this, response);
    }
    else if(keyList.first() == "upload")
    {
        entitiesUploaded_[key] = response;
    }

    if(keyList.first() == "update" && modFuncs_.contains("update"))
    {
        entitiesUpdated_[key] = modFuncs_["update"](this, response);
    }
    else if(keyList.first() == "update")
    {
        entitiesUpdated_[key] = response;
    }

    if(keyList.first() == "delete" && modFuncs_.contains("delete"))
    {
        entitiesDeleted_[key] = modFuncs_["delete"](this, response);
    }
    else if(keyList.first() == "delete")
    {
        entitiesDeleted_[key] = response;
    }

    if(failState_)
    {
        emit failed(failKey_, failReason_);
    }

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, entitiesUploaded_, entitiesUpdated_, entitiesDeleted_);
        reset();
    }
}

void GMAbstractEntity::handleFailure(const QString &key, const QString &reason)
{
    qDebug() << "LocationUpload::handleFailure Fail Key " << key;
    qDebug() << "LocationUpload::handleFailure Fail Reason " << reason;

    failState_ = true;
    failReason_ = reason;
    failKey_ = key;
}

void GMAbstractEntity::processEntities(const QString &key,
                                       const QList<QVariantMap> &argList)
{
    if(!activeJobs_.isEmpty())
    {
        debugMessage("Currently processing location override time windows. Try again once current request is finished.");
        return;
    }

    reset();
    currentKey_ = key;
    QStringList totalKeys;

    for(auto argMap:argList)
    {
        for(auto keys:databaseFuncs_.keys())
        {
            if(key == "upload")
                entitiesToUpload_ = mergeEntities(entitiesToUpload_, databaseFuncs_["upload"](bridgeDB_, argMap));
            if(key == "update")
                entitiesToUpdate_ = mergeEntities(entitiesToUpdate_, databaseFuncs_["update"](bridgeDB_, argMap));
            if(key == "delete")
                entitiesToDelete_ = mergeEntities(entitiesToDelete_, databaseFuncs_["delete"](bridgeDB_, argMap));
        }
    }


    entitiesToUpload_ = prefixEntityKeys("upload", entitiesToUpload_);
    entitiesToUpdate_ = prefixEntityKeys("update", entitiesToUpdate_);
    entitiesToDelete_ = prefixEntityKeys("delete", entitiesToDelete_);

    totalKeys << entitiesToUpload_.keys() << entitiesToUpdate_.keys() << entitiesToDelete_.keys();

    activeJobs_ = QSet<QString>::fromList(totalKeys);
    qDebug() << activeJobs_.size();
    //QList<QString> crash;
    //QString burn = crash.first();

    for(auto key:entitiesToUpload_.keys())
    {
        qDebug() << "IPLOAD" << entitiesToUpload_[key].toObject();
        gmFuncs_["upload"](gmConn_, key, entitiesToUpload_[key].toObject());
    }
    for(auto key:entitiesToUpdate_.keys())
    {
        qDebug() << "IPDATE" << entitiesToUpdate_[key].toObject();
        gmFuncs_["update"](gmConn_, key, entitiesToUpdate_[key].toObject());
    }
    for(auto key:entitiesToDelete_.keys())
    {
        qDebug() << "DILETE" << entitiesToDelete_[key].toObject();
        gmFuncs_["delete"](gmConn_, key, entitiesToDelete_[key].toObject());
    }

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, entitiesUploaded_, entitiesUpdated_, entitiesDeleted_);
        reset();
    }
}

QJsonObject GMAbstractEntity::mergeEntities(QJsonObject initialData, const QJsonObject &additionalData)
{
    for(auto key:additionalData.keys())
    {
        initialData[key] = additionalData[key];
    }
    return initialData;
}

void GMAbstractEntity::reset()
{
    if(!activeJobs_.isEmpty())
    {
        errorMessage("Route deletion in progress. Try again once current request is finished.");
        qDebug() << "Route deletion in progress. Try again once current request is finished.";
        return;
    }

    currentKey_.clear();
    activeJobs_.clear();
    entitiesToUpload_ = QJsonObject();
    entitiesToUpdate_ = QJsonObject();
    entitiesToDelete_ = QJsonObject();
    entitiesUploaded_ = QJsonObject();
    entitiesUpdated_ = QJsonObject();
    entitiesDeleted_ = QJsonObject();
}

QJsonObject GMAbstractEntity::prefixEntityKeys(const QString &prefix, const QJsonObject &entity)
{
    QJsonObject entityCopy;
    for(auto key:entity.keys())
    {
        entityCopy[prefix+":"+key] = QJsonValue(entity[key]);
    }
    return entityCopy;
}

