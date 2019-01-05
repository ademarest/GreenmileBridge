#include "locationtype.h"

LocationType::LocationType(QObject *parent) : GMAbstractEntity(parent)
{

}

void LocationType::processLocationTypes(const QString &key, const QList<QVariantMap> &argList)
{
    databaseFuncs_["upload"]            = &BridgeDatabase::getLocationTypesToUpload;
    internetFuncs_["upload"]            = &GMConnection::uploadLocationType;
    bridgeDataCollectorFuncs_["upload"] = &BridgeDataCollector::handleGMLocationTypes;
    processEntities(key, argList);
}
