#ifndef BRIDGEDATABASE_H
#define BRIDGEDATABASE_H

#include <QtSql>
#include <QtCore>

class BridgeDatabase : public QObject
{
    Q_OBJECT
public:
    explicit BridgeDatabase(QObject *parent = nullptr);

    QJsonArray getRoutesToUpload();
    QJsonArray getRoutesToUpdate();
    QJsonArray getLocationsToUpdate();
    QJsonArray getLocationsToUpload();
    QJsonArray getDriversToUpdate();
    QJsonArray getDriversToUpload();
    QJsonArray getEquipmentToUpdate();
    QJsonArray getEquipmentToUpload();

    bool doesDatabaseExist(const QFile &dbFile);

signals:
    void statusMessage(const QString &dbg);
    void debugMessage(const QString &dbg);
    void errorMessage(const QString &dbg);

public slots:
    void handleAS400RouteQuery(const QMap<QString,QVariantList> &sqlResults);
    void handleGMRouteQuery(const QJsonArray &jsonArray);
    void handleGMOrganizationQuery(const QJsonArray &jsonArray);

private:
    //Utility Section
    QMap<QString,QVariantList> transposeJsonArrayToSQL(const QStringList &expectedKeys, const QJsonArray &data);
    QVariantMap transposeJsonObjectToVarMap(const QStringList &expectedKeys, const QJsonObject &obj);
    QVariant jsonValueToQVariant(const QJsonValue &val);
    QJsonArray transposeSQLToJsonArray(const QMap<QString,QVariantList> &data);

    bool truncateATable(const QString &tableName);
    bool writeToTable(const QString &tableName, QMap<QString, QVariantList> invoiceResults);
    bool executeAQuery(const QString &query, const QString &verb = "unspecified");

    QStringList generateValueTuples(QMap<QString, QVariantList> invoiceResults);
    bool executeQueryAsBatch(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> sqlData);
    bool executeQueryAsString(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> sqlData);

    //Table Creation Section
    void createAS400RouteQueryTable();

    void createMRSDataDriverTable();
    void createMRSDataEquipmentTable();
    void createMRSDataRouteStartTimeTable();
    void createMRSDataOverrideTable();

    void createMRSDailyAssignmentTable();

    void createGMRouteTable();
    void createGMOrganizationTable();
    void createGMEquipmentTable();
    void createGMDriverTable();
    void createGMLocationTable();
    void createGMLocationTimeWindowOverrideTable();
    //End Table Creation Section

    //Table Population Section
    void populateAS400RouteQueryTable(const QMap<QString,QVariantList> &sqlResults);

    void populateMRSDataDriverTable(const QJsonObject &data);
    void populateMRSDataEquipmentTable(const QJsonObject &data);
    void populateMRSDataRouteStartTimeTable(const QJsonObject &data);
    void populateMRSDataOverrideTable(const QJsonObject &data);

    void populateMRSDailyAssignmentTable(const QJsonObject &data);

    void populateGMRouteTable(const QJsonArray &data);
    void populateGMEquipmentTable(const QJsonArray &data);
    void populateGMDriverTable(const QJsonArray &data);
    void populateGMLocationTable(const QJsonArray &data);
    void populateGMLocationTimeWindowOverrideTable(const QJsonArray &data);
    //End Table Population Section

    //Private Variable Section
    QString dbPath_ = qApp->applicationDirPath() + "/bridgeDatabase.db";
};

#endif // BRIDGEDATABASE_H
