#include "servicetimetype.h"

ServiceTimeType::ServiceTimeType(QObject *parent) : GMAbstractEntity(parent)
{

}

void ServiceTimeType::processServiceTimeTypes(const QString &key, const QList<QVariantMap> &argList)
{
    databaseFuncs_["upload"]                = &BridgeDatabase::getServiceTimeTypesToUpload;
    internetFuncs_["upload"]                = &GMConnection::uploadServiceTimeType;
    bridgeDataCollectorFuncs_["upload"]     = &BridgeDataCollector::handleGMServiceTimeTypes;
    processEntities(key, argList);
}
