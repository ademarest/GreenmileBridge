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
    QString operationKey = keyList.first();

    if(keyList.isEmpty())
    {
        emit errorMessage("GMAbstractEntity::handleGMResponse key was empty.");
        return;
    }

    if(postProcessFuncs_.keys().contains(operationKey))
    {
        entitiesProcessed_[operationKey][key] = postProcessFuncs_[operationKey](this, response);
        qDebug() << "Post processing completed.... Operation Key: " << operationKey << "Entity Key:" << key << "Entity:" << entitiesProcessed_[operationKey][key];
    }
    else
    {
        entitiesProcessed_[operationKey][key] = response;
    }

    if(failState_)
    {
        emit failed(failKey_, failReason_);
    }

    if(activeJobs_.empty())
    {
        emit finished(currentKey_, entitiesProcessed_);
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

    qDebug() << "Section 1";
    currentKey_ = key;
    QStringList totalKeys;

    for(auto argMap:argList)
    {
        for(auto operationKey :databaseFuncs_.keys())
        {
            qDebug() << "Section 2" << operationKey;
            entitiesToProcess_[operationKey] = mergeEntities(entitiesToProcess_[operationKey], databaseFuncs_[operationKey](bridgeDB_, argMap));
            entitiesProcessed_[operationKey] = QJsonObject();
        }
    }

    //No mod to key structure.
    for(auto operationKey:entitiesToProcess_.keys())
    {
        qDebug() << "Section 3";
        entitiesToProcess_[operationKey] = prefixEntityKeys(key, entitiesToProcess_[operationKey]);
    }

    //No mod to key structure.
    for(auto operationKey:entitiesToProcess_.keys())
    {
        qDebug() << "Section 4";
        totalKeys << entitiesToProcess_[operationKey].keys();
    }

    activeJobs_ = QSet<QString>::fromList(totalKeys);
    qDebug() << activeJobs_.size();
    //QList<QString> crash;
    //QString burn = crash.first();

    qDebug() << "Section 5";
    for(auto operationKey : entitiesToProcess_.keys())
    {
        if(preprocessFuncs_.keys().contains(operationKey))
        {
            for(auto entityKey : entitiesToProcess_[operationKey].keys())
            {
                qDebug() << "Section 6a";
                entitiesToProcess_[operationKey][entityKey] = preprocessFuncs_[operationKey](this, entitiesToProcess_[operationKey][entityKey]);
                internetFuncs_[operationKey](gmConn_, entityKey,  entitiesToProcess_[operationKey][entityKey].toObject());
                qDebug() << "Operation Key: " << operationKey << "Entity Key" << entityKey << "Entity" << entitiesToProcess_[operationKey][entityKey];
            }
        }
        else
        {
            for(auto entityKey : entitiesToProcess_[operationKey].keys())
            {
                qDebug() << "Section 6b";
                internetFuncs_[operationKey](gmConn_, entityKey,  entitiesToProcess_[operationKey][entityKey].toObject());
                qDebug() << "Operation Key: " << operationKey << "Entity Key" << entityKey << "Entity" << entitiesToProcess_[operationKey][entityKey];
            }
        }
    }


    if(activeJobs_.empty())
    {
        emit finished(currentKey_, entitiesProcessed_);
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
    entitiesProcessed_.clear();
    entitiesToProcess_.clear();
}

QJsonObject GMAbstractEntity::prefixEntityKeys(const QString &prefix, const QJsonObject &entity)
{
    QJsonObject entityCopy;
    //qDebug() << "Merge entities. Entity copy " <<  entityCopy;
    for(auto key:entity.keys())
    {
        entityCopy[prefix+":"+key] = QJsonValue(entity[key]);
    }
    return entityCopy;
}

