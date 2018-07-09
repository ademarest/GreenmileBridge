#include "bridgedatabase.h"

BridgeDatabase::BridgeDatabase(QObject *parent) : QObject(parent)
{
}

void BridgeDatabase::init()
{
    QString as400RouteQueryTableName    = "as400RouteQuery";
    QString gmRouteQueryTableName       = "gmRoutes";
    QString gmOrganizationTableName     = "gmOrganizations";
    QString gmLocationInfoTableName     = "gmLocations";
    QString mrsDailyAssignmentTableName = "mrsDailyAssignments";

    QString as400ROuteQueryCreationQuery = "CREATE TABLE `as400RouteQuery` "
                                           "(`driver:key` TEXT, "
                                           "`equipment:key` TEXT, "
                                           "`location:addressLine1` TEXT, "
                                           "`location:addressLine2` TEXT, "
                                           "`location:city` TEXT, "
                                           "`location:deliveryDays` TEXT, "
                                           "`location:description` TEXT, "
                                           "`location:key` TEXT, "
                                           "`location:state` TEXT, "
                                           "`location:zipCode` TEXT, "
                                           "`locationOverrideTimeWindows:closeTime` TEXT, "
                                           "`locationOverrideTimeWindows:openTime` TEXT, "
                                           "`locationOverrideTimeWindows:tw1Close` TEXT, "
                                           "`locationOverrideTimeWindows:tw1Open` TEXT, "
                                           "`locationOverrideTimeWindows:tw2Close` TEXT, "
                                           "`locationOverrideTimeWindows:tw2Open` TEXT, "
                                           "`order:cube` NUMERIC, "
                                           "`order:number` TEXT NOT NULL UNIQUE, "
                                           "`order:pieces` NUMERIC, "
                                           "`order:weight` NUMERIC, "
                                           "`organization:key` TEXT, "
                                           "`route:date` TEXT, "
                                           "`route:key` TEXT, "
                                           "`stop:plannedSequenceNumber` INT, "
                                           "PRIMARY KEY(`order:number`))";


    QString gmRouteQueryCreationQuery = "CREATE TABLE `gmRoutes` "
                                        "(`date` TEXT, "
                                        "`driverAssignments` TEXT, "
                                        "`driverAssignments:0:driver:key` TEXT, "
                                        "`driversName` TEXT, "
                                        "`equipmentAssignments` TEXT, "
                                        "`equipmentAssignments:0:equipment:key` TEXT, "
                                        "`id` INTEGER NOT NULL UNIQUE, "
                                        "`key` TEXT, "
                                        "`organization` TEXT, "
                                        "`organization:key` TEXT, "
                                        "`organization:id` INTEGER, "
                                        "`stops` TEXT, "
                                        "PRIMARY KEY(`id`))";

    QString gmOrganizationCreationQuery = "CREATE TABLE `gmOrganizations` "
                                          "(`key` TEXT, "
                                          "`description` TEXT, "
                                          "`id` INTEGER NOT NULL UNIQUE, "
                                          "`unitSystem` TEXT, "
                                          "PRIMARY KEY(`id`))";

    QString gmLocationInfoCreationQuery = "CREATE TABLE `gmLocations` "
                                          "(`id` INTEGER NOT NULL UNIQUE, "
                                          "`key` TEXT, "
                                          "`description` TEXT, "
                                          "`addressLine1` TEXT, "
                                          "`addressLine2` TEXT, "
                                          "`city` TEXT, "
                                          "`state` TEXT, "
                                          "`zipCode` TEXT, "
                                          "`latitude` NUMERIC, "
                                          "`longitude` NUMERIC, "
                                          "`geocodingQuality` TEXT, "
                                          "`deliveryDays` TEXT, "
                                          "`enabled` TEXT, "
                                          "`hasGeofence` TEXT, "
                                          "`organization:id` INTEGER, "
                                          "`organization:key` TEXT, "
                                          "`locationOverrideTimeWindows:0:id` INTEGER, "
                                          "`locationType:id` INTEGER, "
                                          "`locationType:key` TEXT, "
                                          "PRIMARY KEY(`id`))";

    QString mrsDailyAssignmentCreationQuery = "CREATE TABLE `mrsDailyAssignments` "
                                              "(`route:key` TEXT NOT NULL, "
                                              "`route:date` TEXT NOT NULL, "
                                              "`organization:key` TEXT NOT NULL, "
                                              "`driver:name` TEXT, "
                                              "`truck:key` TEXT, "
                                              "`trailer:key` TEXT, "
                                              "PRIMARY KEY(`route:key`, `route:date`, `organization:key`))";;

    QStringList gmRouteQueryExpectedKeys {"date",
                                          "driverAssignments:0:driver:key",
                                          "driverAssignments",
                                          "driversName",
                                          "equipmentAssignments:0:equipment:key",
                                          "equipmentAssignments",
                                          "id",
                                          "key",
                                          "organization:key",
                                          "organization:id",
                                          "organization",
                                          "stops"};

    QStringList gmOrganizationExpectedKeys {"id",
                                            "key",
                                            "unitSystem",
                                            "description"};

    QStringList gmLocationInfoExpectedKeys {"id",
                                            "key",
                                            "description",
                                            "addressLine1",
                                            "addressLine2",
                                            "city",
                                            "state",
                                            "zipCode",
                                            "latitude",
                                            "longitude",
                                            "geocodingQuality",
                                            "deliveryDays",
                                            "enabled",
                                            "hasGeofence",
                                            "organization:id",
                                            "organization:key",
                                            "locationOverrideTimeWindows:0:id",
                                            "locationType:id",
                                            "locationType:key"};

    addJsonArrayInfo(gmRouteQueryTableName, gmRouteQueryCreationQuery, gmRouteQueryExpectedKeys);
    addJsonArrayInfo(gmOrganizationTableName, gmOrganizationCreationQuery, gmOrganizationExpectedKeys);
    addJsonArrayInfo(gmLocationInfoTableName, gmLocationInfoCreationQuery, gmLocationInfoExpectedKeys);
    addSQLInfo(as400RouteQueryTableName, as400ROuteQueryCreationQuery);
    addSQLInfo(mrsDailyAssignmentTableName, mrsDailyAssignmentCreationQuery);
}

void BridgeDatabase::SQLDataInsert(const QString &tableName, const QMap<QString, QVariantList> &sql)
{
    if(!okToInsertSQLData(tableName, "SQLDataInsert"))
        return;

    if(!isTableInDB(tableName))
        executeInsertQuery(sqlTableInfoMap_[tableName]["creationQuery"].toString(), QString("create" + tableName));

    writeToTable(tableName, sql);
}

void BridgeDatabase::JSONArrayInsert(const QString &tableName, const QJsonArray &jsonArray)
{
    if(!okToInsertJsonArray(tableName, "JSONArrayInsert"))
        return;

    if(!isTableInDB(tableName))
        executeInsertQuery(jsonTableInfoMap_[tableName]["creationQuery"].toString(), QString("create" + tableName));

    writeToTable(tableName, transposeJsonArrayToSQL(jsonTableInfoMap_[tableName]["expectedKeys"].toStringList(), jsonArray));
}

bool BridgeDatabase::okToInsertJsonArray(const QString &tableName, const QString &whatMethod)
{
    bool ok = false;
    if(tableName.isNull() || tableName.isEmpty())
    {
        emit errorMessage(whatMethod + ": JSON array info cannot be null");
        qDebug() << whatMethod + ": JSON array info cannot be null.";
        return ok;
    }
    if(jsonTableInfoMap_[tableName]["creationQuery"].toString().isNull() || jsonTableInfoMap_[tableName]["creationQuery"].toString().isEmpty())
    {
        emit errorMessage(whatMethod + ": JSON array table creation query cannot be null");
        qDebug() << whatMethod + ": JSON array table creation query cannot be null";
        return ok;
    }
    if(jsonTableInfoMap_[tableName].isEmpty())
    {
        emit errorMessage(whatMethod + ": Expected JSON object keys cannot be null.");
        qDebug() << whatMethod + ": Expected JSON object keys cannot be null.";
        return ok;
    }

    emit statusMessage(whatMethod + ": Created " + tableName +  " table.");
    ok = true;
    return ok;
}

bool BridgeDatabase::okToInsertSQLData(const QString &tableName, const QString &whatMethod)
{
    bool ok = false;
    if(tableName.isNull() || tableName.isEmpty())
    {
        emit errorMessage(whatMethod + ": SQL table info cannot be null");
        qDebug() << whatMethod + ": SQL table info cannot be null.";
        return ok;
    }
    if(sqlTableInfoMap_[tableName]["creationQuery"].toString().isNull() || sqlTableInfoMap_[tableName]["creationQuery"].toString().isEmpty())
    {
        emit errorMessage(whatMethod + ": SQL table creation query cannot be null");
        qDebug() << whatMethod + ": SQL table creation query cannot be null";
        return ok;
    }

    emit statusMessage(whatMethod + ": Created " + tableName +  " table.");
    ok = true;
    return ok;
}

bool BridgeDatabase::isTableInDB(const QString &tableName)
{
    bool existsInDB = false;
    QString verb = "check for table " + tableName;
    QString queryString = "SELECT name FROM sqlite_master WHERE type='table' AND name='"+tableName+"'";

    if(!executeQuery(queryString, verb).isEmpty())
        existsInDB = true;

    return existsInDB;
}

bool BridgeDatabase::addJsonArrayInfo(const QString &tableName, const QString &tableCreationQuery, const QStringList &expectedJsonKeys)
{
    bool ok = false;
    if(tableName.isNull() || tableName.isEmpty())
    {
        emit errorMessage("JSON array info cannot be null");
        qDebug() << "JSON array info cannot be null.";
        return ok;
    }
    if(tableCreationQuery.isNull() || tableCreationQuery.isEmpty())
    {
        emit errorMessage("JSON array table creation query cannot be null");
        qDebug() << "JSON array table creation query cannot be null";
        return ok;
    }
    if(expectedJsonKeys.isEmpty())
    {
        emit errorMessage("Expected JSON object keys cannot be null.");
        qDebug() << "Expected JSON object keys cannot be null.";
        return ok;
    }

    jsonTableInfoMap_[tableName] = QVariantMap();
    jsonTableInfoMap_[tableName]["creationQuery"] = tableCreationQuery;
    jsonTableInfoMap_[tableName]["expectedKeys"] = expectedJsonKeys;

    if(!isTableInDB(tableName))
        executeQuery(jsonTableInfoMap_[tableName]["creationQuery"].toString(), QString(tableName + " creation"));

    if(isTableInDB(tableName))
        ok = true;
    else
        ok = false;

    return ok;
}

bool BridgeDatabase::addSQLInfo(const QString &tableName, const QString &tableCreationQuery)
{
    bool ok = false;
    if(tableName.isNull() || tableName.isEmpty())
    {
        emit errorMessage("SQL table name info cannot be null");
        qDebug() << "SQL table name info cannot be null.";
        return ok;
    }
    if(tableCreationQuery.isNull() || tableCreationQuery.isEmpty())
    {
        emit errorMessage("SQL table creation query cannot be null");
        qDebug() << "SQL table creation query cannot be null";
        return ok;
    }

    sqlTableInfoMap_[tableName] = QVariantMap();
    sqlTableInfoMap_[tableName]["creationQuery"] = tableCreationQuery;

    if(!isTableInDB(tableName))
        executeQuery(sqlTableInfoMap_[tableName]["creationQuery"].toString(), QString(tableName + " creation"));

    if(isTableInDB(tableName))
        ok = true;
    else
        ok = false;

    return ok;
}

QMap<QString, QVariantList> BridgeDatabase::transposeJsonArrayToSQL(const QStringList &expectedKeys, const QJsonArray &data)
{
    QMap<QString, QVariantList> sqlConversion;
    QVariantMap sqlRow;
    QJsonObject jsonObject;

    for(auto jsonValue:data)
    {
        sqlRow = transposeJsonObjectToVarMap(expectedKeys, jsonValue.toObject());
        //qDebug() << sqlRow;
        for(auto key:sqlRow.keys())
        {
            sqlConversion[key].append(sqlRow[key]);
        }
    }
    return sqlConversion;
}

QVariantMap BridgeDatabase::transposeJsonObjectToVarMap(const QStringList &expectedKeys, const QJsonObject &obj)
{
    QVariantMap vMap;
    QJsonValue valCopy;
    QJsonValue subVal;
    QStringList subkeyList;

    for(auto str:expectedKeys)
    {
        valCopy = obj;
        subkeyList = str.split(":");

        if(subkeyList.size() > 1)
        {
            for(int i = 0; i < subkeyList.size(); ++i)
            {
                if(valCopy[subkeyList[i]].isArray())
                {
                    int arrayIdx = subkeyList[i+1].toInt();
                    subVal = valCopy[subkeyList[i]].toArray()[arrayIdx];
                    valCopy = subVal;
                    ++i;
                }
                else if(valCopy[subkeyList[i]].isObject())
                {
                    subVal = valCopy[subkeyList[i]];
                    valCopy = subVal;
                }
                else
                {
                    subVal = valCopy[subkeyList[i]];
                    valCopy = subVal;
                }

                if(i == (subkeyList.size() - 1))
                {
                    vMap[str] = jsonValueToQVariant(valCopy);
                    //qDebug() << str << valCopy;
                    break;
                }
            }
        }
        else
        {
            vMap[str] = jsonValueToQVariant(valCopy[str]);
            //qDebug() << str << valCopy.toObject()[str];
        }
    }
    //    qDebug() << vMap.size();
    //    for(auto key:vMap.keys())
    //        qDebug() << key << vMap[key].type() << vMap["id"];
    return vMap;
}

QVariant BridgeDatabase::jsonValueToQVariant(const QJsonValue &val)
{
    switch(val.type())
    {
    case QJsonValue::Null:
        return QVariant();

    case QJsonValue::Bool:
        return QVariant(val.toInt());

    case QJsonValue::Double:
        return val.toVariant();

    case QJsonValue::String:
        return val.toVariant();

    case QJsonValue::Array:
    {
        QJsonDocument arrayToString;
        arrayToString.setArray(val.toArray());
        return QVariant(QString(arrayToString.toJson()));
    }

    case QJsonValue::Object:
    {
        QJsonDocument objToString;
        objToString.setObject(val.toObject());
        return QVariant(QString(objToString.toJson()));
    }
    case QJsonValue::Undefined:
        return QVariant();
    }
    return QVariant();
}

QJsonArray BridgeDatabase::transposeSQLToJsonArray(const QMap<QString, QVariantList> &data)
{
    qDebug() << data;
    return QJsonArray();
}

bool BridgeDatabase::truncateATable(const QString &tableName)
{
    QString truncateTableQuery = "TRUNCATE " + tableName;
    return executeInsertQuery(truncateTableQuery, "truncate");
}

bool BridgeDatabase::writeToTable(const QString &tableName, QMap<QString, QVariantList> invoiceResults)
{
    bool success = false;
    if(invoiceResults.isEmpty())
        return success;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbPath_);
        db.setDatabaseName(dbPath_);

        emit statusMessage("Beginning SQLite " + tableName + " import.");
        if(db.open())
        {
            success = executeQueryAsString(db,tableName, invoiceResults);
            if(!success)
            {
                success = executeQueryAsBatch(db,tableName, invoiceResults);
            }
        }
        else
        {
            emit errorMessage("Failed to open SQLite database.");
            emit errorMessage(db.lastError().text());
        }

        db.close();
        emit statusMessage("Finished SQLite. INSERT OR REPLACE for database "
                           + dbPath_
                           + " for table "
                           + tableName);
    }
    emit statusMessage("Cleaning up SQLite " + dbPath_ + " connection.");
    QSqlDatabase::removeDatabase(dbPath_);

    return success;
}

QMap<QString, QVariantList> BridgeDatabase::executeQuery(const QString &queryString, const QString &verb)
{

    bool success = false;
    QMap<QString,QVariantList> sqlData;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbPath_);
        db.setDatabaseName(dbPath_);

        emit statusMessage("Beginning SQLite " +verb);
        if(db.open())
        {
            QSqlQuery query(db);
            success = query.exec(queryString);

            if(success)
            {
                while(query.next())
                {
                    for(int j = 0; j < query.record().count(); ++j)
                    {
                        sqlData[query.record().fieldName(j)].append(query.value(j));
                    }
                }
            }
            if(!success)
            {
                emit errorMessage("Failed to execute a "+verb+" type command"
                                  + " on the SQLite database "
                                  + dbPath_
                                  + " for table "
                                  + dbPath_);

                emit errorMessage("Query error: " + query.lastError().text());
            }
        }
        else
        {
            emit errorMessage("Failed to open SQLite database.");
            emit errorMessage(db.lastError().text());
        }

        db.close();
        emit statusMessage("Finished SQLite. "+verb+" completed database "
                           + dbPath_
                           + " for table "
                           + dbPath_);
    }
    emit statusMessage("Cleaning up SQLite " + dbPath_ + " connection.");
    QSqlDatabase::removeDatabase(dbPath_);

    return sqlData;
}

bool BridgeDatabase::executeASYNCQuery(const QString &queryString, const QString &queryKey, const int chunkSize, const QString &verb)
{
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbPath_);
        db.setDatabaseName(dbPath_);

        emit statusMessage("Beginning SQLite " +verb);
        if(db.open())
        {
            QSqlQuery query(db);
            success = query.exec(queryString);
            processASYNCQuery(query, queryKey, chunkSize);
            if(!success)
            {
                emit errorMessage("Failed to execute a "+verb+" type command"
                                  + " on the SQLite database "
                                  + dbPath_
                                  + " for table "
                                  + dbPath_);

                emit errorMessage("Query error: " + query.lastError().text());
            }
        }
        else
        {
            emit errorMessage("Failed to open SQLite database.");
            emit errorMessage(db.lastError().text());
        }

        db.close();
        emit statusMessage("Finished SQLite. "+verb+" completed database "
                           + dbPath_
                           + " for table "
                           + dbPath_);
    }
    emit statusMessage("Cleaning up SQLite " + dbPath_ + " connection.");
    QSqlDatabase::removeDatabase(dbPath_);
    return success;
}

void BridgeDatabase::processASYNCQuery(QSqlQuery &query, const QString &queryKey, const int chunkSize, const QString &verb)
{
    bool firstRun = true;
    int recordCounter = 0;
    QMap<QString,QVariantList> sqlData;
    while(query.next())
    {
        if(recordCounter == chunkSize && chunkSize != 0)
        {
            //Count the amt of records.
            if(sqlData.isEmpty())
            {
                emit debugMessage(verb + ": SQLite query returned an empty result set.");
                qDebug() << verb + ": SQLite query returned an empty result set.";
            }
            else
                emit statusMessage(QString(verb + ": Retrieved " +  QString::number(sqlData.first().size()) + " records from SQLite."));

            emit asyncSqlResults(firstRun, queryKey, sqlData);
            firstRun = false;

            for(auto key: sqlData.keys())
            {
                sqlData[key].clear();
            }
            sqlData.clear();
            recordCounter = 0;
        }

        for(int j = 0; j < query.record().count(); ++j)
        {
            sqlData[query.record().fieldName(j)].append(query.value(j));
        }
        ++recordCounter;
    }

    //Count the amt of records.
    if(sqlData.isEmpty())
    {
        emit debugMessage(verb + ": SQLite query returned an empty result set.");
        qDebug() << verb + ": SQLite query returned an empty result set.";
    }
    else
        emit statusMessage(QString(verb + ": Retrieved " +  QString::number(sqlData.first().size()) + " records from SQLite."));

    emit asyncSqlResults(firstRun, queryKey, sqlData);

    for(auto key: sqlData.keys())
        sqlData[key].clear();

    sqlData.clear();
}

bool BridgeDatabase::executeInsertQuery(const QString &queryString, const QString &verb)
{
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbPath_);
        db.setDatabaseName(dbPath_);

        emit statusMessage("Beginning SQLite " +verb);
        if(db.open())
        {
            QSqlQuery query(db);
            success = query.exec(queryString);
            if(!success)
            {
                emit errorMessage("Failed to execute a "+verb+" type command"
                                  + " on the SQLite database "
                                  + dbPath_
                                  + " for table "
                                  + dbPath_);

                emit errorMessage("Query error: " + query.lastError().text());
            }
        }
        else
        {
            emit errorMessage("Failed to open SQLite database.");
            emit errorMessage(db.lastError().text());
        }

        db.close();
        emit statusMessage("Finished SQLite. "+verb+" completed database "
                           + dbPath_
                           + " for table "
                           + dbPath_);
    }
    emit statusMessage("Cleaning up SQLite " + dbPath_ + " connection.");
    QSqlDatabase::removeDatabase(dbPath_);
    return success;
}

QStringList BridgeDatabase::generateValueTuples(QMap<QString, QVariantList> invoiceResults)
{
    QStringList valueList;
    QStringList valueTuples;
    for(int i = 0; i < invoiceResults.first().size(); ++i)
    {
        valueList.clear();
        for(auto key:invoiceResults.keys())
        {
            if(invoiceResults[key].size() > invoiceResults.first().size())
            {
                emit errorMessage("Cannot assemble value tuples. Index for " + key + " out of range.");
                return QStringList();
            }
            switch(invoiceResults[key][i].type()) {

            case QVariant::Type::Int:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(invoiceResults[key][i].toString());
                break;

            case QVariant::Type::LongLong:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(invoiceResults[key][i].toString());
                break;

            case QVariant::Type::Double:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(invoiceResults[key][i].toString());
                break;

            case QVariant::Type::String:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + invoiceResults[key][i].toString().toLatin1().replace("\"", "\"\"") + "\"");
                break;

            case QVariant::Type::Date:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + invoiceResults[key][i].toDate().toString(Qt::ISODate) + "\"");
                break;

            case QVariant::Type::DateTime:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + invoiceResults[key][i].toDate().toString(Qt::ISODate) + "\"");
                break;

            case QVariant::Type::Time:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + invoiceResults[key][i].toTime().toString(Qt::ISODate) + "\"");
                break;

            case QVariant::Type::Invalid:
                valueList.append("NULL");
                break;

            default:
                qDebug() << "Unknown variant type in switch. "
                            "If you see this a lot, "
                            " your event loop might be getting smashed... " << invoiceResults[key][i].type();
                emit errorMessage(QString("Unsupported data type from SQLite database "
                                          + invoiceResults[key][i].toString()
                                          + " "
                                          + invoiceResults[key][i].type()));
                break;
            }
        }
        valueTuples.append("(" + valueList.join(", ") + ")");
        //qDebug() << valueTuples.last();
    }
    return valueTuples;
}

bool BridgeDatabase::executeQueryAsBatch(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> sqlData)
{
    qDebug() << "batch insert";
    emit statusMessage("Falling back to batch insert.");

    bool success = false;
    int recordCount = 0;
    QString queryString = "INSERT OR REPLACE INTO " + tableName + " (";
    QStringList columnsToUpdate;
    QStringList fieldList;
    QStringList updateStatements;
    QSqlQuery query(db);

    for(auto key:sqlData.keys())
    {
        QString keyTick = "`" + key + "`";
        columnsToUpdate.append(keyTick);
        updateStatements.append(keyTick + "=VALUES(" + keyTick + ")");
        fieldList.append("?");
    }

    recordCount = sqlData.first().size();

    queryString.append(columnsToUpdate.join(",") + ") VALUES (" + fieldList.join(", ") + ")");
    query.prepare(queryString);

    for(auto results:sqlData)
        query.addBindValue(results);

    emit statusMessage("Beginning SQLite fallback batch insert of " + QString::number(recordCount) + " length");

    db.driver()->beginTransaction();

    if(query.execBatch())
        success = true;
    else
        emit errorMessage("Query Error in batch insert. Last error is: " + query.lastError().text());

    db.driver()->commitTransaction();

    if(success)
        emit statusMessage("Completed SQLite batch insert.");
    else
        emit errorMessage("Failed SQLite batch insert.");

    return success;
}

bool BridgeDatabase::executeQueryAsString(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> sqlData)
{
    bool success = false;
    QSqlQuery query(db);
    QStringList valueTuples;
    QString queryString = "INSERT OR REPLACE INTO " + tableName +" (";
    QStringList columnsToUpdate;
    QStringList updateStatements;
    for(auto key:sqlData.keys())
    {
        QString keyTick = "`" + key + "`";
        columnsToUpdate.append(keyTick);
        updateStatements.append(keyTick + "=VALUES(" + keyTick + ")");
    }
    valueTuples = generateValueTuples(sqlData);
    queryString.append(columnsToUpdate.join(", ") + ") VALUES " + valueTuples.join(", "));
    emit statusMessage(QString("Query length for " + tableName + " is " + QString::number(queryString.size()) + " char."));
    emit statusMessage(QString("Inserting " + QString::number(sqlData.first().size()) + " records into " + tableName + "."));

    db.driver()->beginTransaction();

    if(query.exec(queryString))
    {
        success = true;
        emit statusMessage("SQLite string upload to " + tableName + "  completed.");
    }
    else
    {
        success = false;
        emit errorMessage("Error in SQLite string query for " + tableName + ": " + query.lastError().text() + ".");
    }
    db.driver()->commitTransaction();

    return success;
}

