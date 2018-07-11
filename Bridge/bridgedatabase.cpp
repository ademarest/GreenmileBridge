#include "bridgedatabase.h"

BridgeDatabase::BridgeDatabase(QObject *parent) : QObject(parent)
{
}

bool BridgeDatabase::populateAS400LocationOverrideTimeWindows()
{
    return false;
}

QJsonArray BridgeDatabase::getLocationsToUpload(const QString &organizationKey, const QDate &date, const QString &minRouteString, const QString &maxRouteString)
{
    QJsonArray array;
    QString query = "SELECT "
                    "as400RouteQuery.`location:key` as `key`,"
                    "as400RouteQuery.`location:addressLine1` as `addressLine1`,"
                    "as400RouteQuery.`location:addressLine2` as `addressLine2`,"
                    "as400RouteQuery.`location:city` as `city`,"
                    "as400RouteQuery.`location:deliveryDays` as `deliveryDays`,"
                    "as400RouteQuery.`location:description` as `description`,"
                    "as400RouteQuery.`location:key` as `key`,"
                    "as400RouteQuery.`location:state` as `state`,"
                    "as400RouteQuery.`location:zipCode` as `zipCode` "
                    "FROM as400RouteQuery WHERE `organization:key` = \""+organizationKey+"\" AND `route:date` = \""+date.toString(Qt::ISODate)+"\" AND `location:key` NOT IN (SELECT `key` FROM gmLocations) AND `route:key` IN (SELECT `route:key` FROM mrsDailyAssignments WHERE `organization:key` = \""+organizationKey+"\" AND `route:date` = \""+date.toString(Qt::ISODate)+"\" AND `driver:name` IS NOT NULL AND `truck:key` IS NOT NULL AND `route:key` > \""+minRouteString+"\" AND `route:key` < \""+maxRouteString+"\") GROUP BY as400RouteQuery.`location:key`";
    QMap<QString,QVariantList> sql = executeQuery(query, "Finding locations to upload");

    if(!sql.empty())
    {
        for(int i = 0; i < sql.first().size(); ++i)
        {
            QJsonObject obj = QJsonObject();
            for(auto key:sql.keys())
            {
                obj[key] = sql[key][i].toJsonValue();
            }
            obj["enabled"]      = QJsonValue(true);
            obj["locationType"] = QJsonObject{{"id",QJsonValue(10001)}};
            obj["organization"] = QJsonObject{{"id",QJsonValue(10020)}};
            array.append(obj);
        }
    }
    qDebug() << QJsonDocument(array).toJson(QJsonDocument::Compact);
    return array;
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

