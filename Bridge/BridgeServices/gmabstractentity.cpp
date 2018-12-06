#include "gmabstractentity.h"

GMAbstractEntity::GMAbstractEntity(QObject *parent) : QObject(parent)
{
    connect(gmConn_,    &GMConnection::gmNetworkResponse,   this, &GMAbstractEntity::handleGMResponse);

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

void GMAbstractEntity::handleGMResponse(const QString &key, const QJsonValue &response)
{
    activeJobs_.remove(key);
    entitiesProcessed_[key] = response;

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

void GMAbstractEntity::mergeEntities(const QJsonObject &jObj)
{
    for(auto key:jObj.keys())
    {
        entitiesToProcess_[key] = jObj[key];
    }
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
    entitiesToProcess_ = QJsonObject();
    entitiesProcessed_ = QJsonObject();
}

