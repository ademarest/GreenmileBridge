#include "bridgedatabase.h"

BridgeDatabase::BridgeDatabase(QObject *parent) : QObject(parent)
{

}

bool BridgeDatabase::populateAS400LocationOverrideTimeWindows()
{
    return false;
}

QJsonObject BridgeDatabase::getLocationsToUpload(const QString &assignmentTableName,
                                                 const QString &organizationKey,
                                                 const QDate &date,
                                                 const QString &minRouteString,
                                                 const QString &maxRouteString)
{
    //QJsonArray responseArray;
    QString routeKeyBoundaries = " AND `route:key` > \""+minRouteString+"\" AND `route:key` < \""+maxRouteString+"\"";

    if(minRouteString.isNull() || maxRouteString.isNull())
        routeKeyBoundaries = QString();

    QJsonObject locationObj;
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
                    "FROM as400RouteQuery WHERE `organization:key` = \""+organizationKey+"\" AND `route:date` = \""+date.toString(Qt::ISODate)+"\" AND `location:key` NOT IN (SELECT `key` FROM gmLocations) AND `route:key` IN (SELECT `route:key` FROM "+assignmentTableName+" WHERE `organization:key` = \""+organizationKey+"\" AND `route:date` = \""+date.toString(Qt::ISODate)+"\" AND `driver:name` IS NOT NULL AND `truck:key` IS NOT NULL"+routeKeyBoundaries+") GROUP BY as400RouteQuery.`location:key`";

    emit debugMessage(query);
    QMap<QString,QVariantList> sql = executeQuery(query, "Finding locations to upload");
    qDebug() << "sql empty check";
    qDebug() << sql;
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
            locationObj[obj["key"].toString()] = obj;
        }
    }
    else
        qDebug() << "sql empty";

    return locationObj;
}

void BridgeDatabase::enforceTableSanity(QStringList primaryKeyList,
                                        const QString &primaryTable,
                                        const QString &secondaryTable)
{

    for(int i = 0; i < primaryKeyList.size(); ++i)
    {
        primaryKeyList[i].append("`");
        primaryKeyList[i].prepend("`");
    }

    QString query = "DELETE FROM "+secondaryTable+" WHERE "+primaryKeyList.join(" || ")+" IN (SELECT "+primaryKeyList.join(" || ")+" FROM "+primaryTable+" INTERSECT SELECT "+primaryKeyList.join(" || ")+" FROM "+secondaryTable+")";
    executeQuery(query, "Enforcing sanity between " + primaryTable + " and " + secondaryTable);
}

QJsonObject BridgeDatabase::getRoutesToUpload(const QString &assignmentTableName,
                                              const QString &organizationKey,
                                              const QDate &date,
                                              const QString &minRouteString,
                                              const QString &maxRouteString)
{

    QString routeKeyBoundaries = " AND `route:key` > \""+minRouteString+"\" AND `route:key` < \""+maxRouteString+"\"";

    if(minRouteString.isNull() || maxRouteString.isNull())
        routeKeyBoundaries = QString();

    QString query = "SELECT routeQuery.`order:number` as `order:number`, routeQuery.`order:pieces` as `order:plannedSize1`, routeQuery.`order:cube` as `order:plannedSize2`, routeQuery.`order:weight` as `order:plannedSize3`, routeQuery.`route:date`, routeQuery.`route:key`, routeQuery.`stop:baseLineSequenceNum`, gmOrg.`id` as `organization:id`, `gmDriverInfo`.`id` as `driver:id`, `gmEquipmentInfo`.`id` as `equipment:id`, TRIM(routeQuery.`route:date` || \"T\" ||`rst`.`avgStartTime`) as `route:plannedArrival`, TRIM(routeQuery.`route:date` || \"T\" ||`rst`.`avgStartTime`) as `route:plannedComplete`, TRIM(routeQuery.`route:date` || \"T\" ||`rst`.`avgStartTime`) as `route:plannedDeparture`, TRIM(routeQuery.`route:date` || \"T\" ||`rst`.`avgStartTime`) as `route:plannedStart`, `rst`.`avgStartsPrev` AS `startsPreviousDay`, `gmLoc`.`id` as `origin:id`, `gmLoc`.`id` as `destination:id`, `gmLocID`.`id` as `location:id` FROM as400RouteQuery `routeQuery` LEFT JOIN gmOrganizations `gmOrg` ON gmOrg.`key` = routeQuery.`organization:key` LEFT JOIN "+assignmentTableName+" `dailyAssignment` ON `routeQuery`.`route:key` = `dailyAssignment`.`route:key` AND `routeQuery`.`route:date` = `dailyAssignment`.`route:date`AND `routeQuery`.`organization:key` = `dailyAssignment`.`organization:key` LEFT JOIN drivers `mrsDataDrivers` ON `dailyAssignment`.`driver:name` = `mrsDataDrivers`.`employeeName` LEFT JOIN gmDrivers `gmDriverInfo` ON `gmDriverInfo`.`login` = `mrsDataDrivers`.`employeeNumber` LEFT JOIN gmEquipment `gmEquipmentInfo` ON `gmEquipmentInfo`.`key` = `dailyAssignment`.`truck:key` LEFT JOIN routeStartTimes `rst` ON `rst`.`route` = `routeQuery`.`route:key` LEFT JOIN gmLocations `gmLoc` ON `gmLoc`.`key` = `routeQuery`.`organization:key` LEFT JOIN gmLocations `gmLocID` ON `gmLocID`.`key` = `routeQuery`.`location:key`"
                    "WHERE `routeQuery`.`organization:key` = \""+organizationKey+"\" AND `routeQuery`.`route:date` = \""+date.toString("yyyy-MM-dd")+"\" AND `routeQuery`.`route:key` NOT IN (SELECT `key` FROM gmRoutes WHERE `organization:key` = \""+organizationKey+"\" AND `date` = \""+date.toString("yyyy-MM-dd")+"\") AND `routeQuery`.`route:key` IN (SELECT `route:key` FROM "+assignmentTableName+" WHERE `organization:key` = \""+organizationKey+"\" AND `route:date` = \""+date.toString("yyyy-MM-dd")+"\" AND `driver:name` IS NOT NULL AND `truck:key` IS NOT NULL"+routeKeyBoundaries+")";

    emit debugMessage(query);
    QMap<QString,QVariantList> sql = executeQuery(query, "Building routes to upload");
    return assembleUploadRouteFromQuery(sql);
}

QJsonArray BridgeDatabase::getRoutesToUpdate(const QString &assignmentTableName,
                                             const QString &organizationKey,
                                             const QDate &date,
                                             const QString &minRouteString,
                                             const QString &maxRouteString)
{
    qDebug() << assignmentTableName;
    qDebug() << organizationKey;
    qDebug() << date;
    qDebug() << minRouteString;
    qDebug() << maxRouteString;

    return QJsonArray();
}

QJsonObject BridgeDatabase::getAssignmentsToUpdate(const QString &assignmentTableName,
                                                   const QString &organizationKey,
                                                   const QDate &date,
                                                   const QString &minRouteString,
                                                   const QString &maxRouteString)
{
    QString routeKeyBoundaries = " AND `route:key` > \""+minRouteString+"\" AND `route:key` < \""+maxRouteString+"\"";

    if(minRouteString.isNull() || maxRouteString.isNull())
        routeKeyBoundaries = QString();

    QString query = "SELECT gmr.`key`, gmr.`date`, gmr.`organization:id`, gmr.`id`, gmr.`driverAssignments:0:id`, gmDrv.`id` AS `driverAssignments:0:driver:id`, gmEqp.`id` AS `equipmentAssignments:0:equipment:id` , gmr.`equipmentAssignments:0:id` FROM gmRoutes AS gmr LEFT JOIN "+assignmentTableName+" AS mrsda ON mrsda.`route:key` || mrsda.`route:date` || mrsda.`organization:key` = gmr.`key` || gmr.`date` || gmr.`organization:key` LEFT JOIN drivers AS mrsdrv ON mrsda.`driver:name` = mrsdrv.`employeeName` LEFT JOIN gmDrivers AS gmDrv ON mrsdrv.`employeeNumber` = gmDrv.`key` LEFT JOIN gmEquipment AS gmEqp ON gmEqp.`key` = mrsda.`truck:key` WHERE gmr.`status` != 'COMPLETED' AND gmr.`key` IN (SELECT `route:key` FROM (SELECT DISTINCT routeQuery.`route:key`, routeQuery.`route:date`, gmOrg.`id` as `organization:id`, `gmDriverInfo`.`key` as `driver:id`, `gmEquipmentInfo`.`key` as `equipment:id` FROM as400RouteQuery `routeQuery` LEFT JOIN gmOrganizations `gmOrg` ON gmOrg.`key` = routeQuery.`organization:key` LEFT JOIN "+assignmentTableName+" `dailyAssignment` ON `routeQuery`.`route:key` = `dailyAssignment`.`route:key` AND `routeQuery`.`route:date` = `dailyAssignment`.`route:date`AND `routeQuery`.`organization:key` = `dailyAssignment`.`organization:key` LEFT JOIN drivers `mrsDataDrivers` ON `dailyAssignment`.`driver:name` = `mrsDataDrivers`.`employeeName` LEFT JOIN gmDrivers `gmDriverInfo` ON `gmDriverInfo`.`login` = `mrsDataDrivers`.`employeeNumber` LEFT JOIN gmEquipment `gmEquipmentInfo` ON `gmEquipmentInfo`.`key` = `dailyAssignment`.`truck:key` LEFT JOIN routeStartTimes `rst` ON `rst`.`route` = `routeQuery`.`route:key` LEFT JOIN gmLocations `gmLoc` ON `gmLoc`.`key` = `routeQuery`.`organization:key` LEFT JOIN gmLocations `gmLocID` ON `gmLocID`.`key` = `routeQuery`.`location:key`WHERE `routeQuery`.`organization:key` = \""+organizationKey+"\" AND `routeQuery`.`route:date` = \""+date.toString("yyyy-MM-dd")+"\" AND `routeQuery`.`route:key` IN (SELECT `key` FROM gmRoutes WHERE `organization:key` = \""+organizationKey+"\" AND `date` = \""+date.toString("yyyy-MM-dd")+"\") AND `routeQuery`.`route:key` IN (SELECT `route:key` FROM "+assignmentTableName+" WHERE `organization:key` = \""+organizationKey+"\" AND `route:date` = \""+date.toString("yyyy-MM-dd") +"\""+ routeKeyBoundaries+") EXCEPT SELECT DISTINCT `key`, `date`, `organization:id`, `driverAssignments:0:driver:key`, `equipmentAssignments:0:equipment:key` FROM gmRoutes WHERE `organization:key` = '"+organizationKey+"' AND `date` = '"+date.toString("yyyy-MM-dd")+"'))";

    emit debugMessage(query);
    QMap<QString, QVariantList> sql = executeQuery(query, "Getting truck and driver assignment corrections.");
    QJsonObject returnObj;
    if(!sql.empty())
    {
        for(int i = 0; i < sql.first().size(); ++i)
        {
            QString reassignmentKey;
            QStringList reassignmentKeyList;
            QJsonObject reassignmentObj;
            for(auto key:sql.keys())
            {
                reassignmentObj[key] = sql[key][i].toJsonValue();
            }
            reassignmentKeyList << "reassignment"
                                << reassignmentObj["key"].toString()
                                << reassignmentObj["date"].toString()
                                << QString::number(reassignmentObj["organization:id"].toInt());

            reassignmentKey = reassignmentKeyList.join(":");
            returnObj[reassignmentKey] = QJsonValue(reassignmentObj);
        }
    }
    qDebug() << returnObj;
    return returnObj;
}

QJsonObject BridgeDatabase::assembleUploadRouteFromQuery(const QMap<QString,QVariantList> &sql)
{
    //QJsonArray responseArray;
    QJsonObject returnObj;
    QStringList splitKey;
    QMap<QString, QJsonObject> route;
    QMap<QString, QMap<int, QJsonObject>> stops;
    QMap<QString, QJsonObject> organization;
    QMap<QString, QJsonObject> destination;
    QMap<QString, QJsonObject> origin;
    QMap<QString, QJsonObject> driver;
    QMap<QString, QJsonObject> equipment;

    qDebug() << "sql empty check";
    //qDebug() << sql;
    bool startsPrevDay  = false;
    bool runsBackwards   = false;
    if(!sql.empty())
    {
        for(int i = 0; i < sql.first().size(); ++i)
        {
            QJsonObject order;
            QJsonObject location;
            QString routeKey = sql["route:key"][i].toString();
            int stopKey = sql["stop:baseLineSequenceNum"][i].toInt();
            for(auto key:sql.keys())
            {
                splitKey = key.split(":");
                if(splitKey.size() != 2)
                {
                    if(key == "startsPreviousDay")
                    {
                        if(sql[key][i].isNull())
                            startsPrevDay = false;
                        else
                            startsPrevDay = true;
                    }
                }
                if(splitKey.size() == 2)
                {

                    if(splitKey[0] == "route")
                    {
                        if(splitKey[1].contains("planned") && sql[key][i].toDateTime().isValid())
                        {
                            if(startsPrevDay)
                                route[routeKey][splitKey[1]] = QJsonValue(sql[key][i].toDateTime().toUTC().addDays(-1).toString(Qt::ISODate));
                            else
                                route[routeKey][splitKey[1]] = QJsonValue(sql[key][i].toDateTime().toUTC().toString(Qt::ISODate));
                        }
                        else if(splitKey[1].contains("planned") && sql[key][i].isNull())
                        {
                            qDebug() << "start into for route is now";
                            if(startsPrevDay)
                                route[routeKey][splitKey[1]] = QJsonValue(QDateTime::currentDateTimeUtc().addDays(-1).toString(Qt::ISODate));
                            else
                                route[routeKey][splitKey[1]] = QJsonValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                        }
                        else
                        {
                            route[routeKey][splitKey[1]] = sql[key][i].toJsonValue();
                        }
                    }
                    if(splitKey[0] == "origin")
                    {
                        origin[routeKey][splitKey[1]] = sql[key][i].toJsonValue();
                    }
                    if(splitKey[0] == "destination")
                    {
                        destination[routeKey][splitKey[1]] = sql[key][i].toJsonValue();
                    }
                    if(splitKey[0] == "organization")
                    {
                        organization[routeKey][splitKey[1]] = sql[key][i].toJsonValue();
                    }
                    if(splitKey[0] == "location")
                    {
                        location[splitKey[1]] = QJsonValue(sql[key][i].toString());
                    }
                    if(splitKey[0] == "order")
                    {
                        order[splitKey[1]] = sql[key][i].toJsonValue();
                    }
                    if(splitKey[0] == "stop")
                    {
                        stops[routeKey][stopKey][splitKey[1]] = sql[key][i].toJsonValue();
                    }
                    if(splitKey[0] == "driver")
                    {
                        driver[routeKey]["id"] = sql[key][i].toJsonValue();
                    }
                    if(splitKey[0] == "equipment")
                    {
                        equipment[routeKey]["id"] = sql[key][i].toJsonValue();
                    }
                }

            }
            if(splitKey[0] == "stop")
            {
                stops[routeKey][stopKey]["location"] = location;
                stops[routeKey][stopKey]["key"] = QJsonValue(QUuid::createUuid().toString());
                stops[routeKey][stopKey]["stopType"] = QJsonObject{{"id",QJsonValue(10000)}};
                QJsonArray orderArr = stops[routeKey][stopKey]["orders"].toArray();
                orderArr.append(order);
                stops[routeKey][stopKey]["orders"] = QJsonValue(orderArr);
            }
        }
    }

    else
        qDebug() << "sql empty";


    for(auto routeKey:route.keys())
    {
        QString dayOfWeek = QDate::fromString(route[routeKey]["date"].toString(), Qt::ISODate).toString("dddd").toLower();
        QString overrideQuery = "SELECT DISTINCT `gmOrigin`.`id` as `origin`, `gmDestination`.`id` as `destination`, CASE WHEN `rteOver`.`"+dayOfWeek+":backwards` = 'FALSE' THEN 0 ELSE 1 END as `backwards` FROM routeOverrides `rteOver` LEFT JOIN gmLocations `gmOrigin` ON `gmOrigin`.`key` = `rteOver`.`"+dayOfWeek+":origin` LEFT JOIN gmLocations `gmDestination` ON `gmDestination`.`key` = `rteOver`.`"+dayOfWeek+":destination` LEFT JOIN gmOrganizations ON gmOrganizations.`key` = `rteOver`.`organization:key` WHERE gmOrganizations.`id` = "+QString::number(organization[routeKey]["id"].toInt())+" AND `rteOver`.`route:key` = '"+routeKey+"'";
        QMap<QString, QVariantList> sql = executeQuery(overrideQuery);
        if(!sql.isEmpty())
        {
            for(auto overrideKey:sql.keys())
            {
                if(overrideKey == "backwards")
                    runsBackwards = sql[overrideKey].first().toBool();

                if(overrideKey == "origin")
                {
                    origin[routeKey]["id"] = QJsonValue(sql[overrideKey].first().toInt());
                    route[routeKey]["origin"] = origin[routeKey];

                }

                if(overrideKey == "destination")
                {
                    destination[routeKey]["id"] = QJsonValue(sql[overrideKey].first().toInt());
                    route[routeKey]["destination"] = destination[routeKey];
                }
            }
        }
        else
        {
            route[routeKey]["origin"] = origin[routeKey];
            route[routeKey]["destination"] = destination[routeKey];
        }

        route[routeKey]["organization"] = organization[routeKey];
        route[routeKey]["hasHelper"] = QJsonValue(false);
        route[routeKey]["lastStopIsDestination"] = QJsonValue(false);
        QJsonArray stopArr = route[routeKey]["stops"].toArray();

        if(runsBackwards)
        {
            for(auto stopKey:stops[routeKey].keys())
                stopArr.prepend(stops[routeKey][stopKey]);
        }
        else
        {
            for(auto stopKey:stops[routeKey].keys())
                stopArr.append(stops[routeKey][stopKey]);
        }

        route[routeKey]["stops"] = QJsonValue(stopArr);

        QString routeCompoundKey;
        QString driverCompoundKey;
        QString equipmentCompoundKey;
        QStringList genericKeyList;
        genericKeyList  << routeKey
                        << QString::number(route[routeKey]["organization"].toObject()["id"].toInt())
                        << route[routeKey]["date"].toString();


        routeCompoundKey = "routeUpload:" + genericKeyList.join(":");
        driverCompoundKey = "routeDriverAssignment:" + genericKeyList.join(":");
        equipmentCompoundKey = "routeEquipmentAssignment:" + genericKeyList.join(":");


        //qDebug() << route[routeKey];
        returnObj[routeCompoundKey] = route[routeKey];
        returnObj[driverCompoundKey] = driver[routeKey];
        returnObj[equipmentCompoundKey] = equipment[routeKey];
    }
    //qDebug() << returnObj;
    return returnObj;
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
        return val.toVariant();

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
    qDebug() << "transposeSQLToJsonArray not implemented.";
    qDebug() << data;
    return QJsonArray();
}

bool BridgeDatabase::truncateATable(const QString &tableName)
{
    QString truncateTableQuery = "DELETE FROM " + tableName;
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
                success = executeQueryResiliantly(db,tableName, invoiceResults);
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

            case QVariant::Type::Bool:
                if(invoiceResults[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(invoiceResults[key][i].toString());
                break;

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
                qApp->processEvents();
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
        emit errorMessage("SQLite completed batch insert with errors.");

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


bool BridgeDatabase::executeQueryResiliantly(QSqlDatabase &db, const QString &tableName, QMap<QString, QVariantList> sqlData)
{
    bool success = true;
    QSqlQuery query(db);
    QStringList columnsToUpdate;
    for(auto key:sqlData.keys())
    {
        QString keyTick = "`" + key + "`";
        columnsToUpdate.append(keyTick);
    }

    emit statusMessage("Beginning resiliant INSERT for " + tableName);

    for(int i = 0; i < sqlData.first().size(); ++i)
    {
        QMap<QString, QVariantList> singleSqlData;
        for(auto key : sqlData.keys())
        {
            singleSqlData[key].append(sqlData[key][i]);
        }

        QString queryString = "INSERT OR REPLACE INTO " + tableName +" (";
        QStringList valueTuples = generateValueTuples(singleSqlData);

        queryString.append(columnsToUpdate.join(", ") + ") VALUES " + valueTuples.join(", "));
        //emit statusMessage(QString("Query length for " + tableName + " is " + QString::number(queryString.size()) + " char."));
        //emit statusMessage(QString("Inserting " + QString::number(sqlData.first().size()) + " records into " + tableName + "."));

        db.driver()->beginTransaction();

        if(!query.exec(queryString))
        {
            success = false;
            emit errorMessage("Failed on query "  + queryString);
            qDebug() << "Failed on query "  + queryString;
        }

        db.driver()->commitTransaction();
    }
    return success;
}
