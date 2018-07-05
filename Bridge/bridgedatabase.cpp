#include "bridgedatabase.h"

BridgeDatabase::BridgeDatabase(QObject *parent) : QObject(parent)
{
    if(!QFile(dbPath_).exists())
    {
        createAS400RouteQueryTable();
    }
}

void BridgeDatabase::handleAS400RouteQuery(const QMap<QString, QVariantList> &sqlResults)
{
    qDebug() << "handleAS400RouteQuery?";
    writeToTable("as400RouteQuery", sqlResults);
}


QMap<QString, QVariantList> BridgeDatabase::transposeJsonArrayToSQL(const QJsonArray &data)
{

}

QJsonArray BridgeDatabase::transposeSQLToJsonArray(const QMap<QString, QVariantList> &data)
{

}

bool BridgeDatabase::truncateATable(const QString &tableName)
{
    QString truncateTableQuery = "TRUNCATE " + tableName;
    return executeAQuery(truncateTableQuery, "truncate");
}

bool BridgeDatabase::writeToTable(const QString &tableName, QMap<QString, QVariantList> invoiceResults)
{
    bool success = false;
    if(invoiceResults.isEmpty())
        return success;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbPath_);
        db.setDatabaseName(dbPath_);

        emit debugMessage("Beginning SQLite upload");
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
            emit debugMessage("Failed to open SQLite database.");
            emit debugMessage(db.lastError().text());
        }

        db.close();
        emit debugMessage("Finished SQLite. INSERT OR REPLACE for database "
                            + dbPath_
                            + " for table "
                            + tableName);
    }
    qDebug() << "writeToTable out of query";
    emit debugMessage("Cleaning up SQLite " + dbPath_ + " connection.");
    QSqlDatabase::removeDatabase(dbPath_);

    return success;
}

bool BridgeDatabase::executeAQuery(const QString &queryString, const QString &verb)
{
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbPath_);
        db.setDatabaseName(dbPath_);

        emit debugMessage("Beginning SQLite " +verb);
        if(db.open())
        {
            QSqlQuery query(db);
            success = query.exec(queryString);
            if(!success)
            {
                emit debugMessage("Failed to execute a "+verb+" type command"
                                    + " on the SQLite database "
                                    + dbPath_
                                    + " for table "
                                    + dbPath_);

                emit debugMessage("Query error: " + query.lastError().text());
            }
        }
        else
        {
            emit debugMessage("Failed to open SQLite database.");
            emit debugMessage(db.lastError().text());
        }

        db.close();
        emit debugMessage("Finished SQLite. "+verb+" completed database "
                            + dbPath_
                            + " for table "
                            + dbPath_);
    }
    emit debugMessage("Cleaning up SQLite " + dbPath_ + " connection.");
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
                    valueList.append("\"" + invoiceResults[key][i].toString().toLatin1() + "\"");
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

            default:
                qDebug() << "Unknown variant type in switch. "
                            "If you see this a lot, "
                            " your event loop might be getting smashed... " << invoiceResults[key][i].type();
                emit debugMessage(QString("Unsupported data type from AS400 database "
                                          + invoiceResults[key][i].toString()
                                          + " "
                                          + invoiceResults[key][i].type()));
                break;
            }
        }
        valueTuples.append("(" + valueList.join(", ") + ")");
    }
    return valueTuples;
}

bool BridgeDatabase::executeQueryAsBatch(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> sqlData)
{
    qDebug() << "batch inser";
    emit debugMessage("Falling back to batch insert.");

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

    emit debugMessage("Beginning SQLite fallback batch insert of " + QString::number(recordCount) + " length");

    db.driver()->beginTransaction();

    if(query.execBatch())
        success = true;
    else
        emit debugMessage("Query Error in batch insert. Last error is: " + query.lastError().text());

    db.driver()->commitTransaction();

    if(success)
        emit debugMessage("Completed SQLite batch insert.");
    else
        emit debugMessage("Failed SQLite batch insert.");

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
    qDebug() << "before keys";
    for(auto key:sqlData.keys())
    {
        QString keyTick = "`" + key + "`";
        columnsToUpdate.append(keyTick);
        updateStatements.append(keyTick + "=VALUES(" + keyTick + ")");
    }
    qDebug() << "b4 tuples";
    valueTuples = generateValueTuples(sqlData);
    queryString.append(columnsToUpdate.join(", ") + ") VALUES " + valueTuples.join(", "));
    qDebug() << QString("Query length is " + QString::number(queryString.size()) + " char.");


    emit debugMessage(QString("Query length is " + QString::number(queryString.size()) + " char."));
    emit debugMessage(QString("Inserting " + QString::number(sqlData.first().size()) + " records into SQLite"));

    db.driver()->beginTransaction();

    if(query.exec(queryString))
    {
        success = true;
        emit debugMessage("SQLite string upload completed.");
    }
    else
    {
        success = false;
        emit debugMessage("Error in SQLite string query...");
        emit debugMessage(query.lastError().text());
    }
    db.driver()->commitTransaction();

    return success;
}

void BridgeDatabase::createAS400RouteQueryTable()
{
    QString query = "CREATE TABLE `as400RouteQuery` "
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

    executeAQuery(query, "create table");
}
