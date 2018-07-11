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
    QJsonObject getLocationsToUpload(const QString &organizationKey, const QDate &date, const QString &minRouteString, const QString &maxRouteString);
    QJsonArray getDriversToUpdate();
    QJsonArray getDriversToUpload();
    QJsonArray getEquipmentToUpdate();
    QJsonArray getEquipmentToUpload();

    bool populateAS400LocationOverrideTimeWindows();

    void init();
    bool doesDatabaseExist(const QFile &dbFile);
    bool isTableInDB(const QString &tableName);

    bool addJsonArrayInfo(const QString &tableName,
                          const QString &tableCreationQuery,
                          const QStringList &expectedJsonKeys);

    bool addSQLInfo(const QString &tableName,
                    const QString &tableCreationQuery);

signals:
    void statusMessage(const QString &dbg);
    void debugMessage(const QString &dbg);
    void errorMessage(const QString &dbg);
    void asyncSqlResults(const bool isFirstRun,
                         const QString &queryKey,
                         const QMap<QString, QVariantList> &sqlResults);

public slots:
    void JSONArrayInsert(const QString &tableName, const QJsonArray &jsonArray);
    void SQLDataInsert(const QString &tableName, const QMap<QString, QVariantList> &sql);

private:
    //Utility Section

    QMap<QString,QVariantList> transposeJsonArrayToSQL(const QStringList &expectedKeys, const QJsonArray &data);
    QVariantMap transposeJsonObjectToVarMap(const QStringList &expectedKeys, const QJsonObject &obj);
    QVariant jsonValueToQVariant(const QJsonValue &val);
    QJsonArray transposeSQLToJsonArray(const QMap<QString,QVariantList> &data);

    bool truncateATable(const QString &tableName);
    bool writeToTable(const QString &tableName, QMap<QString, QVariantList> invoiceResults);
    bool executeInsertQuery(const QString &query, const QString &verb = "unspecified insert query");
    QMap<QString, QVariantList> executeQuery(const QString &queryString, const QString &verb  = "unspecified query");
    bool executeASYNCQuery(const QString &queryString, const QString &queryKey, const int chunkSize, const QString &verb = "unspecified async query");
    void processASYNCQuery(QSqlQuery &query, const QString &queryKey, const int chunkSize, const QString &verb = "unspecified async query process");

    QStringList generateValueTuples(QMap<QString, QVariantList> invoiceResults);
    bool executeQueryAsBatch(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> sqlData);
    bool executeQueryAsString(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> sqlData);

    //Private Variable Section
    QString dbPath_ = qApp->applicationDirPath() + "/bridgeDatabase.db";
    QMap<QString, QVariantMap> jsonTableInfoMap_;
    QMap<QString, QVariantMap> sqlTableInfoMap_;

    bool okToInsertJsonArray(const QString &tableName, const QString &whatMethod);
    bool okToInsertSQLData(const QString &tableName, const QString &whatMethod);
};

#endif // BRIDGEDATABASE_H
