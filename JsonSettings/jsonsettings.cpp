#include "jsonsettings.h"

JsonSettings::JsonSettings(QObject *parent) : QObject(parent)
{
}

JsonSettings::~JsonSettings()
{

}


QJsonObject JsonSettings::loadSettings(const QFile &dbFile,const QJsonObject &jsonSettings)
{
    QJsonObject populatedJsonSettings;
    QString connectionName = QUuid::createUuid().toString();
    emit debugMessage("Loading JsonSettings.");

    if(!dbFile.fileName().isEmpty() && !jsonSettings.isEmpty())
    {
        if(!doesDatabaseExist(dbFile))
        {
            makeInitalSettingsDatabase(dbFile);
        }

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            db.setDatabaseName(dbFile.fileName());
            if(db.open())
            {
                populatedJsonSettings = selectSettingsFromDB(db,jsonSettings);
            }
            else
            {
                emit debugMessage("Failed to load settings database " +
                                  dbFile.fileName() +
                                  " returning empty settings object");

                return jsonSettings;
            }
            db.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
    }
    else
    {
        emit debugMessage("JsonSettings::loadSettings error, dbPath or JsonObject is null");
        return jsonSettings;
    }

    return populatedJsonSettings;
}

QJsonObject JsonSettings::selectSettingsFromDB(QSqlDatabase &db, QJsonObject jsonSettings)
{
    QSqlQuery query(db);
    QStringList jsonKeys = jsonSettings.keys();
    for(int i=0; i < jsonKeys.size(); ++i)
    {
        jsonKeys[i].append("\"");
        jsonKeys[i].prepend("\"");
    }

    QString queryString  = "SELECT * FROM settings WHERE key IN (" + jsonKeys.join(", ") + ")";
    if(query.exec(queryString))
    {
        emit debugMessage("JsonSettings::loadSettings query success.");

        while(query.next())
        {
            switch(query.value("jsonType").toInt())
            {
            case QJsonValue::Null:
                jsonSettings[query.value("key").toString()] = QJsonValue(QJsonValue::Null);
                break;

            case QJsonValue::Bool:
                jsonSettings[query.value("key").toString()] = QJsonValue(bool(query.value("value").toString().toInt()));
                break;

            case QJsonValue::Double:
                jsonSettings[query.value("key").toString()] = QJsonValue(query.value("value").toString().toDouble());
                break;

            case QJsonValue::String:
                jsonSettings[query.value("key").toString()] = QJsonValue(query.value("value").toString());
                break;

            case QJsonValue::Array:
                jsonSettings[query.value("key").toString()] = QJsonDocument::fromJson(query.value("value").toString().toUtf8()).array();
                break;

            case QJsonValue::Object:
                jsonSettings[query.value("key").toString()] = QJsonDocument::fromJson(query.value("value").toString().toUtf8()).object();
                break;

            case QJsonValue::Undefined:
                jsonSettings[query.value("key").toString()] = QJsonValue(QJsonValue::Undefined);
                break;
            }
        }
    }
    else
        emit debugMessage("JsonSettings::loadSettings ERROR: " + query.lastError().text());
    return jsonSettings;
}

bool JsonSettings::saveSettings(const QFile &dbFile, const QJsonObject &jsonSettings)
{
    bool success = false;
    QString connectionName = QUuid::createUuid().toString();

    if(dbFile.fileName().isEmpty())
        return success;

    if(!doesDatabaseExist(dbFile))
        makeInitalSettingsDatabase(dbFile);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbFile.fileName());
        if(db.open())
        {
            success = insertSettingsIntoDB(db, jsonSettings);
            db.close();
        }
        else
        {
            emit debugMessage("Could not save json object to database. DB error = " + db.lastError().text());
        }
    }
    QSqlDatabase::removeDatabase(connectionName);

    return success;
}

bool JsonSettings::insertSettingsIntoDB(QSqlDatabase &db, const QJsonObject &jsonSettings)
{
    bool success =false;
    QSqlQuery query(db);
    makeSQLQuery(query, jsonSettings);

    db.transaction();
    success = query.execBatch();
    db.commit();

    if(!success)
    {
        emit debugMessage("Could not save json object to database. Query error = " + query.lastError().text());
        qDebug() << query.lastError().text();
    }

    return success;
}


void JsonSettings::makeSQLQuery(QSqlQuery &query, const QJsonObject &jsonSettings)
{
    query.prepare("INSERT OR REPLACE INTO settings VALUES (?,?,?)");

    QVariantList jsonKeys;
    QVariantList jsonValues;
    QVariantList jsonTypes;

    for(auto key: jsonSettings.keys())
    {
        switch(jsonSettings[key].type())
        {
        case QJsonValue::Null:
            jsonKeys.append(key);
            jsonValues.append("null");
            jsonTypes.append(QJsonValue::Null);
            break;

        case QJsonValue::Bool:
            jsonKeys.append(key);
            jsonValues.append(QString::number(int(jsonSettings[key].toBool())));
            jsonTypes.append(QJsonValue::Bool);
            break;

        case QJsonValue::Double:
            jsonKeys.append(key);
            jsonValues.append(QString::number(jsonSettings[key].toDouble()));
            jsonTypes.append(QJsonValue::Double);
            break;

        case QJsonValue::String:
            jsonKeys.append(key);
            jsonValues.append(jsonSettings[key].toString());
            jsonTypes.append(QJsonValue::String);
            break;

        case QJsonValue::Array:
        {
            QJsonDocument arrayToString;
            arrayToString.setArray(jsonSettings[key].toArray());
            jsonKeys.append(key);
            jsonValues.append(QString(arrayToString.toJson()));
            jsonTypes.append(QJsonValue::Array);
        }
            break;

        case QJsonValue::Object:
        {
            QJsonDocument objToString;
            objToString.setObject(jsonSettings[key].toObject());
            jsonKeys.append(key);
            jsonValues.append(QString(objToString.toJson()));
            jsonTypes.append(QJsonValue::Object);
        }
            break;

        case QJsonValue::Undefined:
            jsonKeys.append(key);
            jsonValues.append("Error, undefined");
            jsonTypes.append(QJsonValue::Undefined);
            break;
        }
    }

    query.addBindValue(jsonKeys);
    query.addBindValue(jsonValues);
    query.addBindValue(jsonTypes);
}


bool JsonSettings::doesDatabaseExist(const QFile &dbFile)
{
    if(dbFile.exists())
        return true;
    else
        return false;
}

bool JsonSettings::makeInitalSettingsDatabase(const QFile &dbFile)
{
    bool success = false;
    QString connectionName = QUuid::createUuid().toString();
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);

        db.setDatabaseName(dbFile.fileName());
        if(db.open())
        {
            QSqlQuery query(db);
            success = query.exec("CREATE TABLE settings(key TEXT PRIMARY KEY, value TEXT, jsonType INTEGER)");
            query.finish();
            if(!success)
                emit debugMessage("Failed to exec init query. " + query.lastError().text());
        }
        else
             emit debugMessage("Failed to make init db. DB could not be opened." +  db.lastError().text());

        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    return success;
}
