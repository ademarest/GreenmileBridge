#include "locationoverridetimewindow.h"

LocationOverrideTimeWindow::LocationOverrideTimeWindow(QObject *parent) : GMAbstractEntity(parent)
{

}

void LocationOverrideTimeWindow::processLocationOverrideTimeWindows(const QString &key, const QList<QVariantMap> &argList)
{
    databaseFuncs_["upload"]    = &BridgeDatabase::getLocationOverrideTimeWindowsToUpload;
    databaseFuncs_["update"]    = &BridgeDatabase::getLocationOverrideTimeWindowsToUpdate;
    databaseFuncs_["delete"]    = &BridgeDatabase::getLocationOverrideTimeWindowIDsToDelete;
    gmFuncs_["upload"]          = &GMConnection::uploadALocationOverrideTimeWindow;
    gmFuncs_["update"]          = &GMConnection::updateALocationOverrideTimeWindow;
    gmFuncs_["delete"]          = &GMConnection::deleteALocationOverrideTimeWindow;

    processEntities(key, argList);
}


