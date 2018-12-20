#include "servicetimetype.h"

ServiceTimeType::ServiceTimeType(QObject *parent) : GMAbstractEntity(parent)
{

}

void ServiceTimeType::processAccountTypes(const QString &key, const QList<QVariantMap> &argList)
{
    databaseFuncs_["upload"]    = &BridgeDatabase::getLocationOverrideTimeWindowsToUpload;
    databaseFuncs_["update"]    = &BridgeDatabase::getLocationOverrideTimeWindowsToUpdate;
    databaseFuncs_["delete"]    = &BridgeDatabase::getLocationOverrideTimeWindowIDsToDelete;
    internetFuncs_["upload"]          = &GMConnection::uploadALocationOverrideTimeWindow;
    internetFuncs_["update"]          = &GMConnection::updateALocationOverrideTimeWindow;
    internetFuncs_["delete"]          = &GMConnection::deleteALocationOverrideTimeWindow;
    processEntities(key, argList);
}
