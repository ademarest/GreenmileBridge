#ifndef GMCONNECTION_H
#define GMCONNECTION_H
#include "Bridge/Internet/httpconn.h"

class GMConnection : public HTTPConn
{
    Q_OBJECT
public:
    explicit GMConnection(QObject *parent = nullptr);
    GMConnection(const QString &databaseName, const QString &serverAddress, const QString &username, const QString &password, const QStringList &headers, const int connectionFreqMS, const int maxActiveConnections, QObject *parent);

    virtual ~GMConnection();

    void setServerAddress(const QString &serverAddress);
    void setUsername(const QString &username);
    void setPassword(const QString &password);

    QString getServerAddress() const;
    QString getUsername() const;

    void requestRouteKeysForDate(const QString &key, const QDate &date);
    void requestLocationKeys(const QString &key);
    void requestLocationInfo(const QString &key);
    void requestAllOrganizationInfo(const QString &key);
    void requestAllStopTypeInfo(const QString &key);
    void requestAllLocationTypeInfo(const QString &key);
    void requestRouteComparisonInfo(const QString &key, const QDate &date);
    void requestDriverInfo(const QString &key);
    void requestEquipmentInfo(const QString &key);
    void uploadARoute(const QString &key, const QJsonObject &routeJson);
    void assignDriverToRoute(const QString &key, const QJsonObject &routeDriverAssignmentJson);
    void assignEquipmentToRoute(const QString &key, const QJsonObject &routeEquipmentAssignmentJson);
    void requestAccountTypes(const QString &key);
    void uploadAccountType(const QString &key, const QJsonObject &accountTypeJson);
    void requestServiceTimeTypes(const QString &key);
    void uploadServiceTimeType(const QString &key, const QJsonObject &serviceTimeTypeJson);
    void requestLocationTypes(const QString &key);
    void uploadLocationType(const QString &key, const QJsonObject &locationTypeJson);
    void requestStopTypes(const QString &key);
    void uploadStopType(const QString &key, const QJsonObject &stopTypeJson);

    void deleteRoute(const QString &key, const QString &entityID);
    void deleteDriverAssignment(const QString &key, const QString &entityID);
    void deleteEquipmentAssignment(const QString &key, const QString &entityID);

    void geocodeLocation(const QString &key, const QJsonObject &locationJson);
    void uploadALocation(const QString &key, const QJsonObject &locationJson);
    void putLocation(const QString &key, const QString &entityID, const QJsonObject &locationJson);
    void patchLocation(const QString &key, const QJsonObject &locationJson);

    bool isProcessingNetworkRequests();

    void requestLocationOverrideTimeWindowInfo(const QString &key);
    void uploadALocationOverrideTimeWindow(const QString &key, const QJsonObject &data);
    void updateALocationOverrideTimeWindow(const QString &key, const QJsonObject &data);
    void deleteALocationOverrideTimeWindow(const QString &key, const QJsonObject &data);
};

#endif // GMCONNECTION_H
