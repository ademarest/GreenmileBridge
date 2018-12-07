#include "locationoverridetimewindow.h"

LocationOverrideTimeWindow::LocationOverrideTimeWindow(QObject *parent) : GMAbstractEntity(parent)
{

}

void LocationOverrideTimeWindow::processLocationOverrideTimeWindows(const QString &key, const QList<QVariantMap> &argList)
{
    std::function<QJsonObject(BridgeDatabase*, QVariantMap)> getUploadsFromDatabaseFunc = &BridgeDatabase::getLocationOverrideTimeWindowsToUpload;
    std::function<QJsonObject(BridgeDatabase*, QVariantMap)> getUpdatesFromDatabaseFunc = &BridgeDatabase::getLocationOverrideTimeWindowsToUpdate;
    std::function<QJsonObject(BridgeDatabase*, QVariantMap)> getDeletesFromDatabaseFunc = &BridgeDatabase::getLocationOverrideTimeWindowIDsToDelete;
    std::function<void(GMConnection*, QString, QJsonObject)> uploadFunc                 = &GMConnection::uploadALocationOverrideTimeWindow;
    std::function<void(GMConnection*, QString, QJsonObject)> updateFunc                 = &GMConnection::updateALocationOverrideTimeWindow;
    std::function<void(GMConnection*, QString, QJsonObject)> deleteFunc                 = &GMConnection::deleteALocationOverrideTimeWindow;

    processEntities(key,
                    argList,
                    getUploadsFromDatabaseFunc,
                    getUpdatesFromDatabaseFunc,
                    getDeletesFromDatabaseFunc,
                    uploadFunc,
                    updateFunc,
                    deleteFunc);
}


