#include "jsonsettings.h"

JsonSettings::JsonSettings(QObject *parent) : QObject(parent)
{
}


QJsonObject JsonSettings::loadSettings(const QFile &dbFile,const QJsonObject &jsonSettings)
{
    QJsonObject populatedJsonSettings;
    emit debugMessage("Loading JsonSettings.");

    if(!dbFile.fileName().isEmpty() && !jsonSettings.isEmpty())
    {
        if(!doesDatabaseExist(dbFile))
        {
            makeInitalSettingsDatabase(dbFile);
        }

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbFile.fileName());
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
        QSqlDatabase::removeDatabase(dbFile.fileName());
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
    if(dbFile.fileName().isEmpty())
        return success;

    if(!doesDatabaseExist(dbFile))
        makeInitalSettingsDatabase(dbFile);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbFile.fileName());
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
    QSqlDatabase::removeDatabase(dbFile.fileName());

    return success;
}

bool JsonSettings::insertSettingsIntoDB(QSqlDatabase &db, const QJsonObject &jsonSettings)
{
    bool success =false;
    QSqlQuery query(db);
    QString queryString = makeInsertString(jsonSettings);
    success = query.exec(queryString);

    if(!success)
        emit debugMessage("Could not save json object to database. Query error = " + query.lastError().text());

    return success;
}


QString JsonSettings::makeInsertString(const QJsonObject &jsonSettings)
{
    QString queryString = "INSERT or REPLACE into settings VALUES ";
    QStringList values;
    QStringList valueTuples;
    for(auto key: jsonSettings.keys())
    {
        values.clear();
        switch(jsonSettings[key].type())
        {
        case QJsonValue::Null:
            values << QString("\"" + key + "\"") << QString("\"null\"")  << QString::number(QJsonValue::Null);
            valueTuples.append("(" + values.join(", ") + ")");
            break;

        case QJsonValue::Bool:
            values << QString("\"" + key + "\"") << QString("\"" + jsonSettings[key].toString() + "\"")  << QString::number(QJsonValue::Bool);
            valueTuples.append("(" + values.join(", ") + ")");
            break;

        case QJsonValue::Double:
            values << QString("\"" + key + "\"") << QString("\"" + jsonSettings[key].toString() + "\"")  << QString::number(QJsonValue::Double);
            valueTuples.append("(" + values.join(", ") + ")");
            break;

        case QJsonValue::String:
            values << QString("\"" + key + "\"") << QString("\"" + jsonSettings[key].toString() + "\"")  << QString::number(QJsonValue::String);
            valueTuples.append("(" + values.join(", ") + ")");
            break;

        case QJsonValue::Array:
        {
            QJsonDocument arrayToString;
            arrayToString.setArray(jsonSettings[key].toArray());
            values << QString("\"" + key + "\"") << QString("\"" + QString(arrayToString.toJson()) + "\"") << QString::number(QJsonValue::Array);
            valueTuples.append("(" + values.join(", ") + ")");
        }
            break;

        case QJsonValue::Object:
        {
            QJsonDocument objToString;
            objToString.setObject(jsonSettings[key].toObject());
            values << QString("\"" + key + "\"") << QString("\"" + QString(objToString.toJson()) + "\"")  << QString::number(QJsonValue::Object);
            valueTuples.append("(" + values.join(", ") + ")");
        }
            break;

        case QJsonValue::Undefined:
            values << QString("\"" + key + "\"") << QString("\"error\"")  << QString::number(QJsonValue::Undefined);
            valueTuples.append("(" + values.join(", ") + ")");
            break;
        }
    }
    queryString.append(valueTuples.join(", "));
    return queryString;
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
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbFile.fileName());

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
    QSqlDatabase::removeDatabase(dbFile.fileName());
    return success;
}
