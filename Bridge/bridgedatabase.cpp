#include "bridgedatabase.h"

BridgeDatabase::BridgeDatabase(QObject *parent) : QObject(parent)
{

}

BridgeDatabase::~BridgeDatabase()
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
    if(!locationsExist())
    {
        return QJsonObject();
    }

    QString routeKeyBoundaries = " AND `route:key` > \""+minRouteString+"\" AND `route:key` < \""+maxRouteString+"\"";

    if(minRouteString.isNull() || maxRouteString.isNull())
        routeKeyBoundaries = QString();

    QJsonObject locationObj;

    QString query = "select \n"
                    "	as400LocationQuery.`location:key` as `key`, \n"
                    "	as400LocationQuery.`location:addressLine1` as `addressLine1`, \n"
                    "	as400LocationQuery.`location:addressLine2` as `addressLine2`, \n"
                    "	as400LocationQuery.`location:city` as `city`, \n"
                    "	as400LocationQuery.`location:deliveryDays` as `deliveryDays`, \n"
                    "	as400LocationQuery.`location:description` as `description`, \n"
                    "	as400LocationQuery.`location:state` as `state`, \n"
                    "	as400LocationQuery.`location:zipCode` as `zipCode`, \n"
                    "	gmOrganizations.`id` as `organization:id` \n"
                    "from \n"
                    "	as400LocationQuery \n"
                    "left join \n"
                    "	gmOrganizations \n"
                    "on \n"
                    "	gmOrganizations.`key` = as400LocationQuery.`organization:key` \n"
                    "where \n"
                    "	`organization:key` = \""+organizationKey+"\" \n"
                    "and `location:key` not in \n"
                    "( \n"
                    "	select `key` from gmLocations \n"
                    ") \n"
                    "and as400LocationQuery.`"+"location:"+date.toString("dddd").toLower()+"Route` in \n"
                    "( \n"
                    "	select \n"
                    "		`route:key` \n"
                    "	from "+assignmentTableName+" \n"
                    "	where \n"
                    "		`organization:key` = \""+organizationKey+"\" \n"
                    "	and \n"
                    "		`route:date` = \""+date.toString("yyyy-MM-dd")+"\" \n"
                    ") \n"
                    "group by as400LocationQuery.`location:key` \n";

    emit debugMessage(query);
    QMap<QString,QVariantList> sql = executeQuery(query, "Finding locations to upload in in getLocationsToUpload()");
    if(!sql.empty())
    {
        for(int i = 0; i < sql.first().size(); ++i)
        {
            QJsonObject obj = QJsonObject();
            QJsonObject organizationObj = QJsonObject();

            for(auto key:sql.keys())
            {
                QStringList splitKey = key.split(":");
                if(splitKey.first() == "organization"){
                    organizationObj[splitKey.last()] = sql[key][i].toJsonValue();
                }
                if(splitKey.length() == 1){
                    obj[key] = sql[key][i].toJsonValue();
                }
            }

            for(auto key:sql.keys())
            {

            }
            obj["enabled"]      = QJsonValue(true);
            obj["locationType"] = QJsonObject{{"id",QJsonValue(10001)}};
            obj["organization"] = organizationObj;
            locationObj[obj["key"].toString()] = obj;
        }
    }
    else
        qDebug() << "sql empty";

    return locationObj;
}

bool BridgeDatabase::locationsExist()
{
    QString gmTestQuery     = "SELECT COUNT(`id`) as `test` FROM gmLocations";
    QString as400TestQuery  = "SELECT COUNT(`location:key`) as `test` FROM as400LocationQuery";

    QMap<QString,QVariantList> gmTestSQL    = executeQuery(gmTestQuery,     "determine if GM locations exist.");
    QMap<QString,QVariantList> as400TestSQL = executeQuery(as400TestQuery,  "determine if AS400 locations exist.");

    if(gmTestSQL["test"].first().toInt() == 0)
    {
        //emit errorMessage("Error in Bridge Database: Greenmile Locations are empty.");
        //have to work out why this fails and crashes...
        emit failed("Bridge database error.", "Greenmile locations were empty.");
        return false;
    }

    if(as400TestSQL["test"].first().toInt() == 0)
    {
        //emit errorMessage("Error in Bridge Database: AS400 Locations are empty.");
        //have to work out why this fails and crashes...
        emit failed("Bridge database error.", "AS400 locations were empty.");
        return false;
    }

    return true;
}

QJsonObject BridgeDatabase::getStopsToDelete(const QString &assignmentTableName,
                                              const QString &organizationKey,
                                              const QDate &date)
{
    //QString query = "SELECT routeQuery.`order:number` as `order:number`, routeQuery.`order:pieces` as `order:plannedSize1`, routeQuery.`order:cube` as `order:plannedSize2`, routeQuery.`order:weight` as `order:plannedSize3`, routeQuery.`route:date`, routeQuery.`route:key`, routeQuery.`stop:baseLineSequenceNum`, gmOrg.`id` as `organization:id`, `gmDriverInfo`.`id` as `driver:id`, `gmEquipmentInfo`.`id` as `equipment:id`, TRIM(routeQuery.`route:date` || \"T\" ||`rst`.`avgStartTime`) as `route:plannedArrival`, TRIM(routeQuery.`route:date` || \"T\" ||`rst`.`avgStartTime`) as `route:plannedComplete`, TRIM(routeQuery.`route:date` || \"T\" ||`rst`.`avgStartTime`) as `route:plannedDeparture`, TRIM(routeQuery.`route:date` || \"T\" ||`rst`.`avgStartTime`) as `route:plannedStart`, `rst`.`avgStartsPrev` AS `startsPreviousDay`, `gmLoc`.`id` as `origin:id`, `gmLoc`.`id` as `destination:id`, `gmLocID`.`id` as `location:id` FROM as400RouteQuery `routeQuery` LEFT JOIN gmOrganizations `gmOrg` ON gmOrg.`key` = routeQuery.`organization:key` LEFT JOIN dl"+assignmentTableName+" `dailyAssignment` ON `routeQuery`.`route:key` = `dailyAssignment`.`route:key` AND `routeQuery`.`route:date` = `dailyAssignment`.`route:date` AND `routeQuery`.`organization:key` = `dailyAssignment`.`organization:key` LEFT JOIN drivers `mrsDataDrivers` ON `dailyAssignment`.`driver:name` = `mrsDataDrivers`.`employeeName` LEFT JOIN gmDrivers `gmDriverInfo` ON `gmDriverInfo`.`login` = `mrsDataDrivers`.`employeeNumber` LEFT JOIN gmEquipment `gmEquipmentInfo` ON `gmEquipmentInfo`.`key` = `dailyAssignment`.`truck:key` LEFT JOIN routeStartTimes `rst` ON `rst`.`route` = `routeQuery`.`route:key` LEFT JOIN gmLocations `gmLoc` ON `gmLoc`.`key` = `routeQuery`.`organization:key` LEFT JOIN gmLocations `gmLocID` ON `gmLocID`.`key` = `routeQuery`.`location:key` WHERE `routeQuery`.`organization:key` = \""+organizationKey+"\" AND `routeQuery`.`route:date` = \"2018-10-17\" AND `routeQuery`.`route:key` IN (SELECT `key` FROM gmRoutes WHERE `organization:key` = \""+organizationKey+"\" AND `date` = \"2018-10-17\") ";
    QString query = "SELECT `id` FROM gmRoutes WHERE `date` || `key` || `organization:id` IN ( SELECT `unique_id` FROM( SELECT CAST(baselineSize1 as INTEGER), CAST(baselineSize2 as INTEGER), CAST(baselineSize3 as INTEGER), `date`, `key`, `organization:id`, `date` || `key` || `organization:id` as `unique_id`, `totalStops` FROM gmRoutes WHERE `date` = '"+date.toString("yyyy-MM-dd")+"' AND `organization:key` = '"+organizationKey+"' AND `key` IN (SELECT `route:key` FROM "+assignmentTableName+" WHERE `organization:key` = '"+organizationKey+"' AND `route:date` = '"+date.toString("yyyy-MM-dd")+"') AND `status` = 'NOT_STARTED' EXCEPT SELECT CAST(SUM(routeQuery.`order:pieces`) as INTEGER) as `order:plannedSize1`, CAST(SUM(routeQuery.`order:cube`) as INTEGER) as `order:plannedSize2`, CAST(SUM(routeQuery.`order:weight`) as INTEGER) as `order:plannedSize3`, routeQuery.`route:date`, routeQuery.`route:key`, gmOrg.`id` as `organization:id`, routeQuery.`route:date` || routeQuery.`route:key` || gmOrg.`id` as `unique_id`, count( distinct `routeQuery`.`location:key`) FROM as400RouteQuery `routeQuery` LEFT JOIN gmOrganizations `gmOrg` ON gmOrg.`key` = routeQuery.`organization:key` LEFT JOIN "+assignmentTableName+" `dailyAssignment` ON `routeQuery`.`route:key` = `dailyAssignment`.`route:key` AND `routeQuery`.`route:date` = `dailyAssignment`.`route:date` AND `routeQuery`.`organization:key` = `dailyAssignment`.`organization:key` LEFT JOIN drivers `mrsDataDrivers` ON `dailyAssignment`.`driver:name` = `mrsDataDrivers`.`employeeName` LEFT JOIN gmDrivers `gmDriverInfo` ON `gmDriverInfo`.`login` = `mrsDataDrivers`.`employeeNumber` LEFT JOIN gmEquipment `gmEquipmentInfo` ON `gmEquipmentInfo`.`key` = `dailyAssignment`.`truck:key` LEFT JOIN routeStartTimes `rst` ON `rst`.`route` = `routeQuery`.`route:key` LEFT JOIN gmLocations `gmLoc` ON `gmLoc`.`key` = `routeQuery`.`organization:key` LEFT JOIN gmLocations `gmLocID` ON `gmLocID`.`key` = `routeQuery`.`location:key` WHERE `routeQuery`.`organization:key` = '"+organizationKey+"' AND `routeQuery`.`route:date` = '"+date.toString("yyyy-MM-dd")+"' AND `routeQuery`.`route:key` IN (SELECT `key` FROM gmRoutes WHERE `organization:key` = '"+organizationKey+"' AND `date` = '"+date.toString("yyyy-MM-dd")+"') AND `routeQuery`.`route:key` IN (SELECT `route:key` FROM "+assignmentTableName+" WHERE `organization:key` = '"+organizationKey+"' AND `route:date` = '"+date.toString("yyyy-MM-dd")+"' ) GROUP BY routeQuery.`route:key`))";

    QMap<QString, QVariantList> sql = executeQuery(query, "Determining stops to delete. BridgeDatabase:getStopsToDelete()");
    QJsonObject assembledAS400Routes = assembleUploadRouteFromQuery(sql);
    for(auto key:assembledAS400Routes.keys())
    {
        if(key.contains("routeUpload"))
        {
            qDebug() << assembledAS400Routes[key];
        }
    }

    qDebug() << "keeyz" << assembledAS400Routes.keys();
    return QJsonObject();
}

QJsonObject BridgeDatabase::getLocationOverrideTimeWindowsToUpload(QVariantMap args)
{
    QString assignmentTableName = args["tableName"].toString();
    QString organizationKey = args["organization:key"].toString();
    QDate   date = args["date"].toDate();

    QString query = "SELECT\n"
                    "  `locationOverrideTimeWindows:0:id` AS `id`,\n"
                    "  `id` AS `location:id`,\n"
                    "  `location:key`,\n"
                    "  `locationOverrideTimeWindows:openTime` AS `openTime`,\n"
                    "  `locationOverrideTimeWindows:closeTime` AS `closeTime`,\n"
                    "  `locationOverrideTimeWindows:tw1Open` AS `tw1Open`,\n"
                    "  `locationOverrideTimeWindows:tw1Close` AS `tw1Close`,\n"
                    "  `locationOverrideTimeWindows:tw2Open` AS `tw2Open`,\n"
                    "  `locationOverrideTimeWindows:tw2Close` AS `tw2Close`,\n"
                    "  `monday`,\n"
                    "  `tuesday`,\n"
                    "  `wednesday`,\n"
                    "  `thursday`,\n"
                    "  `friday`,\n"
                    "  `saturday`,\n"
                    "  `sunday`\n"
                    "FROM\n"
                    "  (\n"
                    "    SELECT\n"
                    "      gmLocations.`locationOverrideTimeWindows:0:id`,\n"
                    "      gmLocations.`id`,\n"
                    "      gmLocations.`key` AS `location:key`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:openTime`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:closeTime`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:tw1Open`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:tw1Close`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:tw2Open`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:tw2Close`,\n"
                    "      as400Combine.`monday`,\n"
                    "      as400Combine.`tuesday`,\n"
                    "      as400Combine.`wednesday`,\n"
                    "      as400Combine.`thursday`,\n"
                    "      as400Combine.`friday`,\n"
                    "      as400Combine.`saturday`,\n"
                    "      as400Combine.`sunday`\n"
                    "    FROM\n"
                    "      (\n"
                    "        SELECT\n"
                    "          as400RouteQuery.`location:key`,\n"
                    "          SUBSTR(as400RouteQuery.`locationOverrideTimeWindows:openTime`,0,6) AS `locationOverrideTimeWindows:openTime`,\n"
                    "          SUBSTR(as400RouteQuery.`locationOverrideTimeWindows:closeTime`,0,6) AS `locationOverrideTimeWindows:closeTime`,\n"
                    "          SUBSTR(CASE WHEN as400RouteQuery.`locationOverrideTimeWindows:tw1Open` IS NULL THEN as400RouteQuery.`locationOverrideTimeWindows:openTime` ELSE as400RouteQuery.`locationOverrideTimeWindows:tw1Open` END,0,6) AS `locationOverrideTimeWindows:tw1Open`,\n"
                    "          SUBSTR(CASE WHEN as400RouteQuery.`locationOverrideTimeWindows:tw1Close` IS NULL THEN as400RouteQuery.`locationOverrideTimeWindows:closeTime` ELSE as400RouteQuery.`locationOverrideTimeWindows:tw1Close` END,0,6) AS `locationOverrideTimeWindows:tw1Close`,\n"
                    "          SUBSTR(as400RouteQuery.`locationOverrideTimeWindows:tw2Open`,0,6) AS `locationOverrideTimeWindows:tw2Open`,\n"
                    "          SUBSTR(as400RouteQuery.`locationOverrideTimeWindows:tw2Close`,0,6) AS `locationOverrideTimeWindows:tw2Close`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%M%\" THEN 1 ELSE 0 END AS `monday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%T%\" THEN 1 ELSE 0 END AS `tuesday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%W%\" THEN 1 ELSE 0 END AS `wednesday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%R%\" THEN 1 ELSE 0 END AS `thursday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%F%\" THEN 1 ELSE 0 END AS `friday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%S%\" THEN 1 ELSE 0 END AS `saturday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%U%\" THEN 1 ELSE 0 END AS `sunday`\n"
                    "        FROM\n"
                    "          as400RouteQuery\n"
                    "        WHERE\n"
                    "          `organization:key` = \""+organizationKey+"\"\n"
                    "          AND `route:date` = \""+date.toString("yyyy-MM-dd")+"\"\n"
                    "          AND `location:key`\n"
                    "          AND `route:key` IN (\n"
                    "            SELECT\n"
                    "              `route:key`\n"
                    "            FROM\n"
                    "              "+assignmentTableName+"\n"
                    "            WHERE\n"
                    "              `organization:key` = \""+organizationKey+"\"\n"
                    "              AND `route:date` = \""+date.toString("yyyy-MM-dd")+"\"\n"
                    "          )\n"
                    "        UNION\n"
                    "        SELECT\n"
                    "          as400LocationQuery.`location:key`,\n"
                    "          SUBSTR(as400LocationQuery.`locationOverrideTimeWindows:openTime`,0,6) AS `locationOverrideTimeWindows:openTime`,\n"
                    "          SUBSTR(as400LocationQuery.`locationOverrideTimeWindows:closeTime`,0,6) AS `locationOverrideTimeWindows:closeTime`,\n"
                    "          SUBSTR(CASE WHEN as400LocationQuery.`locationOverrideTimeWindows:tw1Open` IS NULL THEN as400LocationQuery.`locationOverrideTimeWindows:openTime` ELSE as400LocationQuery.`locationOverrideTimeWindows:tw1Open` END,0,6) AS `locationOverrideTimeWindows:tw1Open`,\n"
                    "          SUBSTR(CASE WHEN as400LocationQuery.`locationOverrideTimeWindows:tw1Close` IS NULL THEN as400LocationQuery.`locationOverrideTimeWindows:closeTime` ELSE as400LocationQuery.`locationOverrideTimeWindows:tw1Close` END,0,6) AS `locationOverrideTimeWindows:tw1Close`,\n"
                    "          SUBSTR(as400LocationQuery.`locationOverrideTimeWindows:tw2Open`,0,6) AS `locationOverrideTimeWindows:tw2Open`,\n"
                    "          SUBSTR(as400LocationQuery.`locationOverrideTimeWindows:tw2Close`,0,6) AS `locationOverrideTimeWindows:tw2Close`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%M%\" THEN 1 ELSE 0 END AS `monday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%T%\" THEN 1 ELSE 0 END AS `tuesday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%W%\" THEN 1 ELSE 0 END AS `wednesday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%R%\" THEN 1 ELSE 0 END AS `thursday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%F%\" THEN 1 ELSE 0 END AS `friday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%S%\" THEN 1 ELSE 0 END AS `saturday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%U%\" THEN 1 ELSE 0 END AS `sunday`\n"
                    "        FROM\n"
                    "          as400LocationQuery\n"
                    "        WHERE\n"
                    "          as400LocationQuery.`location:key` IN (\n"
                    "            SELECT\n"
                    "              `key`\n"
                    "            FROM\n"
                    "              gmLocations\n"
                    "          )\n"
                    "      ) AS as400Combine\n"
                    "      JOIN gmLocations ON gmLocations.`key` = as400Combine.`location:key`\n"
                    "    EXCEPT\n"
                    "    SELECT\n"
                    "      gmLocationOverrideTimeWindows.`id`,\n"
                    "      gmLocations.`id`,\n"
                    "      gmLocations.`key` AS `location:key`,\n"
                    "      gmLocationOverrideTimeWindows.`openTime`,\n"
                    "      gmLocationOverrideTimeWindows.`closeTime`,\n"
                    "      gmLocationOverrideTimeWindows.`tw1Open`,\n"
                    "      gmLocationOverrideTimeWindows.`tw1Close`,\n"
                    "      gmLocationOverrideTimeWindows.`tw2Open`,\n"
                    "      gmLocationOverrideTimeWindows.`tw2Close`,\n"
                    "      gmLocationOverrideTimeWindows.`monday`,\n"
                    "      gmLocationOverrideTimeWindows.`tuesday`,\n"
                    "      gmLocationOverrideTimeWindows.`wednesday`,\n"
                    "      gmLocationOverrideTimeWindows.`thursday`,\n"
                    "      gmLocationOverrideTimeWindows.`friday`,\n"
                    "      gmLocationOverrideTimeWindows.`saturday`,\n"
                    "      gmLocationOverrideTimeWindows.`sunday`\n"
                    "    FROM\n"
                    "      gmLocationOverrideTimeWindows\n"
                    "      JOIN gmLocations\n"
                    "    WHERE\n"
                    "      gmLocations.`id` = gmLocationOverrideTimeWindows.`location:id`\n"
                    "  )\n"
                    "WHERE\n"
                    "  `locationOverrideTimeWindows:0:id` IS NULL\n"
                    "  AND `openTime` IS NOT NULL\n"
                    "  AND `closeTime` IS NOT NULL";

    qDebug() << "BridgeDatabase:getLocationOverrideTimeWindowsToUpload query " << query;

    QMap<QString, QVariantList> sql = executeQuery(query, " find Loction Override Time Windows to upload. BridgeDatabase:getLocationOverrideTimeWindowsToUpload()");
    if(!isSQLResultValid(sql))
    {
        emit statusMessage("There are no Loction Override Time Windows to update. Returning empty result set.");
        return QJsonObject();
    }
    else
    {
        return assembleLocationOverrideTimeWindowFromQuery(sql);
    }
}


QJsonObject BridgeDatabase::getLocationOverrideTimeWindowsToUpdate(QVariantMap args)
{
    QString assignmentTableName = args["tableName"].toString();
    QString organizationKey = args["organization:key"].toString();
    QDate   date = args["date"].toDate();

    QString query = "SELECT\n"
                    "  `locationOverrideTimeWindows:0:id` AS `id`,\n"
                    "  `id` AS `location:id`,\n"
                    "  `location:key`,\n"
                    "  `locationOverrideTimeWindows:openTime` AS `openTime`,\n"
                    "  `locationOverrideTimeWindows:closeTime` AS `closeTime`,\n"
                    "  `locationOverrideTimeWindows:tw1Open` AS `tw1Open`,\n"
                    "  `locationOverrideTimeWindows:tw1Close` AS `tw1Close`,\n"
                    "  `locationOverrideTimeWindows:tw2Open` AS `tw2Open`,\n"
                    "  `locationOverrideTimeWindows:tw2Close` AS `tw2Close`,\n"
                    "  `monday`,\n"
                    "  `tuesday`,\n"
                    "  `wednesday`,\n"
                    "  `thursday`,\n"
                    "  `friday`,\n"
                    "  `saturday`,\n"
                    "  `sunday`\n"
                    "FROM\n"
                    "  (\n"
                    "    SELECT\n"
                    "      gmLocations.`locationOverrideTimeWindows:0:id`,\n"
                    "      gmLocations.`id`,\n"
                    "      gmLocations.`key` AS `location:key`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:openTime`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:closeTime`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:tw1Open`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:tw1Close`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:tw2Open`,\n"
                    "      as400Combine.`locationOverrideTimeWindows:tw2Close`,\n"
                    "      as400Combine.`monday`,\n"
                    "      as400Combine.`tuesday`,\n"
                    "      as400Combine.`wednesday`,\n"
                    "      as400Combine.`thursday`,\n"
                    "      as400Combine.`friday`,\n"
                    "      as400Combine.`saturday`,\n"
                    "      as400Combine.`sunday`\n"
                    "    FROM\n"
                    "      (\n"
                    "        SELECT\n"
                    "          as400RouteQuery.`location:key`,\n"
                    "          SUBSTR(as400RouteQuery.`locationOverrideTimeWindows:openTime`,0,6) AS `locationOverrideTimeWindows:openTime`,\n"
                    "          SUBSTR(as400RouteQuery.`locationOverrideTimeWindows:closeTime`,0,6) AS `locationOverrideTimeWindows:closeTime`,\n"
                    "          SUBSTR(CASE WHEN as400RouteQuery.`locationOverrideTimeWindows:tw1Open` IS NULL THEN as400RouteQuery.`locationOverrideTimeWindows:openTime` ELSE as400RouteQuery.`locationOverrideTimeWindows:tw1Open` END,0,6) AS `locationOverrideTimeWindows:tw1Open`,\n"
                    "          SUBSTR(CASE WHEN as400RouteQuery.`locationOverrideTimeWindows:tw1Close` IS NULL THEN as400RouteQuery.`locationOverrideTimeWindows:closeTime` ELSE as400RouteQuery.`locationOverrideTimeWindows:tw1Close` END,0,6) AS `locationOverrideTimeWindows:tw1Close`,\n"
                    "          SUBSTR(as400RouteQuery.`locationOverrideTimeWindows:tw2Open`,0,6) AS `locationOverrideTimeWindows:tw2Open`,\n"
                    "          SUBSTR(as400RouteQuery.`locationOverrideTimeWindows:tw2Close`,0,6) AS `locationOverrideTimeWindows:tw2Close`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%M%\" THEN 1 ELSE 0 END AS `monday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%T%\" THEN 1 ELSE 0 END AS `tuesday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%W%\" THEN 1 ELSE 0 END AS `wednesday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%R%\" THEN 1 ELSE 0 END AS `thursday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%F%\" THEN 1 ELSE 0 END AS `friday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%S%\" THEN 1 ELSE 0 END AS `saturday`,\n"
                    "          CASE WHEN as400RouteQuery.`location:deliveryDays` LIKE \"%U%\" THEN 1 ELSE 0 END AS `sunday`\n"
                    "        FROM\n"
                    "          as400RouteQuery\n"
                    "        WHERE\n"
                    "          `organization:key` = \""+organizationKey+"\"\n"
                    "          AND `route:date` = \""+date.toString("yyyy-MM-dd")+"\"\n"
                    "          AND `location:key`\n"
                    "          AND `route:key` IN (\n"
                    "            SELECT\n"
                    "              `route:key`\n"
                    "            FROM\n"
                    "              "+assignmentTableName+"\n"
                    "            WHERE\n"
                    "              `organization:key` = \""+organizationKey+"\"\n"
                    "              AND `route:date` = \""+date.toString("yyyy-MM-dd")+"\"\n"
                    "          )\n"
                    "        UNION\n"
                    "        SELECT\n"
                    "          as400LocationQuery.`location:key`,\n"
                    "          SUBSTR(as400LocationQuery.`locationOverrideTimeWindows:openTime`,0,6) AS `locationOverrideTimeWindows:openTime`,\n"
                    "          SUBSTR(as400LocationQuery.`locationOverrideTimeWindows:closeTime`,0,6) AS `locationOverrideTimeWindows:closeTime`,\n"
                    "          SUBSTR(CASE WHEN as400LocationQuery.`locationOverrideTimeWindows:tw1Open` IS NULL THEN as400LocationQuery.`locationOverrideTimeWindows:openTime` ELSE as400LocationQuery.`locationOverrideTimeWindows:tw1Open` END,0,6) AS `locationOverrideTimeWindows:tw1Open`,\n"
                    "          SUBSTR(CASE WHEN as400LocationQuery.`locationOverrideTimeWindows:tw1Close` IS NULL THEN as400LocationQuery.`locationOverrideTimeWindows:closeTime` ELSE as400LocationQuery.`locationOverrideTimeWindows:tw1Close` END,0,6) AS `locationOverrideTimeWindows:tw1Close`,\n"
                    "          SUBSTR(as400LocationQuery.`locationOverrideTimeWindows:tw2Open`,0,6) AS `locationOverrideTimeWindows:tw2Open`,\n"
                    "          SUBSTR(as400LocationQuery.`locationOverrideTimeWindows:tw2Close`,0,6) AS `locationOverrideTimeWindows:tw2Close`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%M%\" THEN 1 ELSE 0 END AS `monday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%T%\" THEN 1 ELSE 0 END AS `tuesday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%W%\" THEN 1 ELSE 0 END AS `wednesday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%R%\" THEN 1 ELSE 0 END AS `thursday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%F%\" THEN 1 ELSE 0 END AS `friday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%S%\" THEN 1 ELSE 0 END AS `saturday`,\n"
                    "          CASE WHEN as400LocationQuery.`location:deliveryDays` LIKE \"%U%\" THEN 1 ELSE 0 END AS `sunday`\n"
                    "        FROM\n"
                    "          as400LocationQuery\n"
                    "        WHERE\n"
                    "          as400LocationQuery.`location:key` IN (\n"
                    "            SELECT\n"
                    "              `key`\n"
                    "            FROM\n"
                    "              gmLocations\n"
                    "          )\n"
                    "      ) AS as400Combine\n"
                    "      JOIN gmLocations ON gmLocations.`key` = as400Combine.`location:key`\n"
                    "    EXCEPT\n"
                    "    SELECT\n"
                    "      gmLocationOverrideTimeWindows.`id`,\n"
                    "      gmLocations.`id`,\n"
                    "      gmLocations.`key` AS `location:key`,\n"
                    "      gmLocationOverrideTimeWindows.`openTime`,\n"
                    "      gmLocationOverrideTimeWindows.`closeTime`,\n"
                    "      gmLocationOverrideTimeWindows.`tw1Open`,\n"
                    "      gmLocationOverrideTimeWindows.`tw1Close`,\n"
                    "      gmLocationOverrideTimeWindows.`tw2Open`,\n"
                    "      gmLocationOverrideTimeWindows.`tw2Close`,\n"
                    "      gmLocationOverrideTimeWindows.`monday`,\n"
                    "      gmLocationOverrideTimeWindows.`tuesday`,\n"
                    "      gmLocationOverrideTimeWindows.`wednesday`,\n"
                    "      gmLocationOverrideTimeWindows.`thursday`,\n"
                    "      gmLocationOverrideTimeWindows.`friday`,\n"
                    "      gmLocationOverrideTimeWindows.`saturday`,\n"
                    "      gmLocationOverrideTimeWindows.`sunday`\n"
                    "    FROM\n"
                    "      gmLocationOverrideTimeWindows\n"
                    "      JOIN gmLocations\n"
                    "    WHERE\n"
                    "      gmLocations.`id` = gmLocationOverrideTimeWindows.`location:id`\n"
                    "  )\n"
                    "WHERE\n"
                    "  `locationOverrideTimeWindows:0:id` IS NOT NULL\n"
                    "  AND `openTime` IS NOT NULL\n"
                    "  AND `closeTime` IS NOT NULL";

    qDebug() << "BridgeDatabase:getLocationOverrideTimeWindowsToUpdate query " << query;
    QMap<QString, QVariantList> sql = executeQuery(query, " find Loction Override Time Windows to update. BridgeDatabase:getLocationOverrideTimeWindowsToUpdate()");
    if(!isSQLResultValid(sql))
    {
        emit statusMessage("There are no Loction Override Time Windows to update. Returning empty result set.");
        return QJsonObject();
    }
    else
    {
        return assembleLocationOverrideTimeWindowFromQuery(sql);
    }
}

QJsonObject BridgeDatabase::getLocationOverrideTimeWindowIDsToDelete(QVariantMap args)
{
    qDebug() << args;
    QString keyBase = "locationOverrideTimeWindowIDToDelete:";
    QJsonObject jObj;
    QString query = "SELECT \n"
                    "`id`\n"
                    "FROM\n"
                    "gmLocationOverrideTimeWindows\n"
                    "WHERE \n"
                    "`location:id`  IS NULL\n"
                    "OR `openTime`  IS NULL\n"
                    "OR `closeTime` IS NULL\n";

    QMap<QString, QVariantList> sql = executeQuery(query, " find Loction Override Time Window IDs to delete. BridgeDatabase:getLocationOverrideTimeWindowIDsToDelete()");

    for(auto var:sql["id"])
    {
        QJsonObject idObj = {{"id", var.toInt()}};
        jObj[keyBase + var.toString()] = QJsonValue(idObj);
    }
    qDebug() << "To delete" << jObj;
    return jObj;
}

QJsonObject BridgeDatabase::getServiceTimeTypesToUpload(QVariantMap args)
{
    int nonHelperFixedTimeSecs  = 300;
    int nonHelperVarTimeSecs    = 24;
    int helperFixedTimeSecs     = 300;
    int helperVarTimeSecs       = 24;

    QString keyBase = "serviceTimeTypeToUpload";
    QJsonObject jObj;
    QString assignmentTableName = args["tableName"].toString();
    QString organizationKey = args["organization:key"].toString();
    QDate date = args["date"].toDate();

    QString query = "SELECT DISTINCT \n"
                    "gmOrganizations.`id` as `organization:id`, \n"
                    "`serviceTimeType:key` as `key`, \n"
                    "`serviceTimeType:key` as `description` \n"
                    "FROM \n"
                    "( \n"
                    "    SELECT \n"
                    "    as400RouteQuery.`serviceTimeType:key` \n"
                    "    FROM as400RouteQuery \n"
                    "    WHERE `organization:key` = '"+organizationKey+"' \n"
                    "    AND `route:date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "    AND `location:key` \n"
                    "    AND `route:key` \n"
                    "    IN \n"
                    "    ( \n"
                    "        SELECT \n"
                    "        `route:key` \n"
                    "        FROM \n"
                    "        "+assignmentTableName+" \n"
                    "        WHERE `organization:key` = '"+organizationKey+"' \n"
                    "        AND `route:date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "    ) \n"
                    "    UNION ALL \n"
                    "    SELECT \n"
                    "    as400LocationQuery.`serviceTimeType:key` \n"
                    "    FROM as400LocationQuery \n"
                    "    WHERE as400LocationQuery.`location:key` \n"
                    "    IN \n"
                    "    ( \n"
                    "        SELECT \n"
                    "        `key` \n"
                    "        FROM \n"
                    "        gmLocations \n"
                    "    ) \n"
                    ") \n"
                    "JOIN \n"
                    "gmOrganizations \n"
                    "ON \n"
                    "gmOrganizations.key = '"+organizationKey+"' \n"
                    "WHERE \n"
                    "`serviceTimeType:key` \n"
                    "NOT IN \n"
                    "( \n"
                    "    SELECT \n"
                    "    `key` \n"
                    "    FROM \n"
                    "    gmServiceTimeTypes \n"
                    "    WHERE \n"
                    "    gmServicetimetypes.`key` IS NOT NULL \n"
                    ") \n";

    qDebug() << "ServiceTimeType to upload query" << query;

    QMap<QString, QVariantList> sql = executeQuery(query, " find Service Time Types to upload. BrdigeDatabase::getServiceTimeTypesToUpload");

    if(!isSQLResultValid(sql))
    {
        emit statusMessage("Service Time Types SQL was empty. Returning empty json object.");
        return jObj;
    }

    for(int i = 0; i < sql.first().size(); ++i)
    {
        QString key = "serviceTimeType:" + sql["organization:id"][i].toString() + ":" + sql["key"][i].toString();
        QJsonObject orgObj = {{"id", sql["organization:id"][i].toJsonValue()}};
        QJsonObject serviceTimeObj =   {{"organization", orgObj},
                                        {"description", sql["description"][i].toJsonValue()},
                                        {"key", sql["key"][i].toJsonValue()},
                                        {"nonHelperFixedTimeSecs", QJsonValue(nonHelperFixedTimeSecs)},
                                        {"nonHelperVarTimeSecs", QJsonValue(nonHelperVarTimeSecs)},
                                        {"helperFixedTimeSecs", QJsonValue(helperFixedTimeSecs)},
                                        {"helperVarTimeSecs", QJsonValue(helperVarTimeSecs)}};

        qDebug() << "Service time to upload" << QJsonDocument(serviceTimeObj).toJson(QJsonDocument::Compact);

        jObj[key] = QJsonValue(serviceTimeObj);
    }

    return jObj;
}

QJsonObject BridgeDatabase::getStopTypesToUpload(const QString &organizationKey)
{
    qDebug() << "BridgeDatabase::getStopTypesToUpload not implemented yet. Org key " << organizationKey;
    return QJsonObject();
}

QJsonObject BridgeDatabase::getLocationTypesToUpload(QVariantMap args)
{
    qDebug() << args;
    QString keyBase = "locationsTypesToUpload:";
    QJsonObject jObj;
    QString assignmentTableName = args["tableName"].toString();
    QString organizationKey = args["organization:key"].toString();
    QDate   date = args["date"].toDate();

    QString query = "SELECT DISTINCT \n"
                    "gmOrganizations.`id` as `organization:id`, \n"
                    "`locationType:key` as `key`, \n"
                    "`locationType:key` as `description` \n"
                    "FROM \n"
                    "( \n"
                    "    SELECT \n"
                    "    as400RouteQuery.`locationType:key` \n"
                    "    FROM as400RouteQuery \n"
                    "    WHERE `organization:key` = '"+organizationKey+"' \n"
                    "    AND `route:date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "    AND `location:key` \n"
                    "    AND `route:key` \n"
                    "    IN \n"
                    "    ( \n"
                    "        SELECT \n"
                    "        `route:key` \n"
                    "        FROM \n"
                    "        "+assignmentTableName+" \n"
                    "        WHERE `organization:key` = '"+organizationKey+"' \n"
                    "        AND `route:date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "    ) \n"
                    "    UNION ALL \n"
                    "    SELECT \n"
                    "    as400LocationQuery.`locationType:key` \n"
                    "    FROM as400LocationQuery \n"
                    "    WHERE as400LocationQuery.`location:key` \n"
                    "    IN \n"
                    "    ( \n"
                    "        SELECT \n"
                    "        `key` \n"
                    "        FROM \n"
                    "        gmLocations \n"
                    "    ) \n"
                    ") \n"
                    "JOIN \n"
                    "gmOrganizations \n"
                    "ON \n"
                    "gmOrganizations.key = '"+organizationKey+"' \n"
                    "WHERE \n"
                    "`locationType:key` \n"
                    "NOT IN \n"
                    "( \n"
                    "    SELECT \n"
                    "    `key` \n"
                    "    FROM \n"
                    "    gmLocationTypes \n"
                    ") \n";

    qDebug() << "Location type to upload query" << query;

    QMap<QString, QVariantList> sql = executeQuery(query, " find Location Types to upload. BrdigeDatabase::getLocationTypesToUpload");

    if(!isSQLResultValid(sql))
    {
        emit statusMessage("Location type SQL was empty. Returning empty json object.");
        return jObj;
    }

    for(int i = 0; i < sql.first().size(); ++i)
    {
        QString key = "accountType:" + sql["organization:id"][i].toString() + ":" + sql["key"][i].toString();
        QJsonObject orgObj      = {{"id", sql["organization:id"][i].toJsonValue()}};
        QJsonObject locTypeObj  =   {{"organization", orgObj},
                                    {"description", sql["description"][i].toJsonValue()},
                                    {"key", sql["key"][i].toJsonValue()},
                                    {"showOnMobileCreate", QJsonValue(true)},
                                    {"enabled", QJsonValue(true)}};
        qDebug() << "Location type to upload json" << locTypeObj;
        jObj[key] = QJsonValue(locTypeObj);
    }
    return jObj;
}

QJsonObject BridgeDatabase::getAccountTypesToUpload(QVariantMap args)
{
    qDebug() << args;
    QString keyBase = "locationOverrideTimeWindowIDToDelete:";
    QJsonObject jObj;
    QString assignmentTableName = args["tableName"].toString();
    QString organizationKey = args["organization:key"].toString();
    QDate   date = args["date"].toDate();

    QString query = "SELECT DISTINCT \n"
                    "gmOrganizations.`id` as `organization:id`, \n"
                    "`accountType:key` as `key`, \n"
                    "`accountType:key` as `description` \n"
                    "FROM \n"
                    "( \n"
                    "    SELECT \n"
                    "    as400RouteQuery.`accountType:key` \n"
                    "    FROM as400RouteQuery \n"
                    "    WHERE `organization:key` = '"+organizationKey+"' \n"
                    "    AND `route:date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "    AND `location:key` \n"
                    "    AND `route:key` \n"
                    "    IN \n"
                    "    ( \n"
                    "        SELECT \n"
                    "        `route:key` \n"
                    "        FROM \n"
                    "        "+assignmentTableName+" \n"
                    "        WHERE `organization:key` = '"+organizationKey+"' \n"
                    "        AND `route:date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "    ) \n"
                    "    UNION ALL \n"
                    "    SELECT \n"
                    "    as400LocationQuery.`accountType:key` \n"
                    "    FROM as400LocationQuery \n"
                    "    WHERE as400LocationQuery.`location:key` \n"
                    "    IN \n"
                    "    ( \n"
                    "        SELECT \n"
                    "        `key` \n"
                    "        FROM \n"
                    "        gmLocations \n"
                    "    ) \n"
                    ") \n"
                    "JOIN \n"
                    "gmOrganizations \n"
                    "ON \n"
                    "gmOrganizations.key = '"+organizationKey+"' \n"
                    "WHERE \n"
                    "`accountType:key` \n"
                    "NOT IN \n"
                    "( \n"
                    "    SELECT \n"
                    "    `key` \n"
                    "    FROM \n"
                    "    gmAccountTypes \n"
                    ") \n";

    qDebug() << "AccountType to upload query" << query;

    QMap<QString, QVariantList> sql = executeQuery(query, " find Account Types to upload. BrdigeDatabase::getAccountTypesToUpload");

    if(!isSQLResultValid(sql))
    {
        emit statusMessage("Account type SQL was empty. Returning empty json object.");
        return jObj;
    }

    for(int i = 0; i < sql.first().size(); ++i)
    {
        QString key = "accountType:" + sql["organization:id"][i].toString() + ":" + sql["key"][i].toString();
        QJsonObject orgObj = {{"id", sql["organization:id"][i].toJsonValue()}};
        QJsonObject acctObj = {{"organization", orgObj},
                               {"description", sql["description"][i].toJsonValue()},
                               {"key", sql["key"][i].toJsonValue()}};

        jObj[key] = QJsonValue(acctObj);
    }

    return jObj;
}


QJsonObject BridgeDatabase::getGMLocationsWithBadGeocode(const QString &organizationKey)
{
    QString query = "SELECT `id` as `location:id`, `key` AS `location:key`,`description` AS `location:description`,`addressLine1` AS `location:addressLine1`,`addressLine2` AS `location:addressLine2`,`city` AS `location:city`, `state` AS `location:state`,`zipCode` AS `location:zipCode`, `organization:id`, `locationType:id` FROM gmLocations WHERE gmLocations.`organization:key` = '"+organizationKey+"' AND gmLocations.`geocodingQuality` = 'UNSUCCESSFULL'";
    //QString query = "SELECT `id` as `location:id`, `key` AS `location:key`,`description` AS `location:description`,`addressLine1` AS `location:addressLine1`,`addressLine2` AS `location:addressLine2`,`city` AS `location:city`, `state` AS `location:state`,`zipCode` AS `location:zipCode`, `organization:id`, `locationType:id` FROM gmLocations WHERE gmLocations.`organization:key` = '"+organizationKey+"' AND gmLocations.`latitude` = 0";

    QMap<QString, QVariantList> sql = executeQuery(query, "Getting locations to update in Greenmile. BridgeDatabase::getGMLocationsWithBadGeocode()");
    QJsonObject returnObj;

    if(!sql.empty())
    {
        emit statusMessage("Getting locations with bad geocodes. There are " + QString::number(sql.first().size()) + " locations to geocode.");
        for(int i = 0; i < sql.first().size(); ++i)
        {
            QString locationKey;
            QStringList locationKeyList;

            QJsonObject locationObj;
            QJsonObject locationTypeObj;
            QJsonObject organizationObj;

            for(auto key:sql.keys())
            {
                QStringList splitKey = key.split(":");
                if(splitKey.first() == "location")
                    locationObj[splitKey.last()] = sql[key][i].toJsonValue();
                if(splitKey.first() == "locationType")
                    locationTypeObj[splitKey.last()] = sql[key][i].toJsonValue();
                if(splitKey.first() == "organization")
                    organizationObj[splitKey.last()] = sql[key][i].toJsonValue();
            }

            locationKeyList << "locationToUpdate"
                                << QString::number(locationObj["id"].toInt())
                                << QString::number(organizationObj["id"].toInt());

            locationKey = locationKeyList.join(":");
            locationObj["organization"] =   QJsonValue(organizationObj);
            locationObj["locationType"] =   QJsonValue(locationTypeObj);
            returnObj[locationKey]      =   QJsonValue(locationObj);
            //qDebug() << locationObj;
        }
    }
    else
    {
        emit statusMessage("There are no locations with bad geocodes to update.");
    }

    return returnObj;
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
                    "WHERE `routeQuery`.`organization:key` = \""+organizationKey+"\" AND `routeQuery`.`route:date` = \""+date.toString("yyyy-MM-dd")+"\" AND `routeQuery`.`route:key` NOT IN (SELECT `key` FROM gmRoutes WHERE `organization:key` = \""+organizationKey+"\" AND `date` = \""+date.toString("yyyy-MM-dd")+"\") AND `routeQuery`.`route:key` IN (SELECT `route:key` FROM "+assignmentTableName+" WHERE `organization:key` = \""+organizationKey+"\" AND `route:date` = \""+date.toString("yyyy-MM-dd")+"\" "+routeKeyBoundaries+")";

    qDebug() << "looking for SELECT RC" <<  query;
    emit debugMessage(query);
    QMap<QString,QVariantList> sql = executeQuery(query, "Building routes to upload");
    qDebug() << "route upload sql size" << sql.size();
    return assembleUploadRouteFromQuery(sql);
}

QJsonObject BridgeDatabase::getRoutesToDelete(const QString &assignmentTableName,
                                             const QString &organizationKey,
                                             const QDate &date)
{
    QJsonObject returnObj;
    QString keyBase = "DeleteRoute:" + organizationKey + ":" + date.toString("yyyy-MM-dd")+ ":";

    QString query = "SELECT \n"
                    "`id` \n"
                    "FROM \n"
                    "gmRoutes \n"
                    "WHERE `date` || `key` || `organization:id` IN \n"
                    "( \n"
                    "    SELECT \n"
                    "    `unique_id` \n"
                    "    FROM \n"
                    "    ( \n"
                    "        SELECT \n"
                    "        ROUND(baselineSize1, 0) AS `baselineSize1`, \n"
                    "        ROUND(baselineSize2, 0) AS `baselineSize2`, \n"
                    "        ROUND(baselineSize3, 0) AS `baselineSize3`, \n"
                    "        `date`, \n"
                    "        `key`, \n"
                    "        `organization:id`, \n"
                    "        `date` || `key` || `organization:id` as `unique_id`, \n"
                    "        `totalStops` \n"
                    "        FROM gmRoutes \n"
                    "        WHERE \n"
                    "        `date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "        AND `organization:key` = '"+organizationKey+"' \n"
                    "        AND `key` \n"
                    "        IN \n"
                    "        ( \n"
                    "            SELECT \n"
                    "            `route:key` \n"
                    "            FROM "+assignmentTableName+" \n"
                    "            WHERE `organization:key` = '"+organizationKey+"' \n"
                    "            AND `route:date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "        ) \n"
                    "        AND `status` = 'NOT_STARTED' \n"
                    "        EXCEPT \n"
                    "        SELECT \n"
                    "        ROUND(SUM(routeQuery.`order:pieces`), 0) as `order:plannedSize1`, \n"
                    "        ROUND(SUM(routeQuery.`order:cube`), 0) as `order:plannedSize2`, \n"
                    "        ROUND(SUM(routeQuery.`order:weight`), 0) as `order:plannedSize3`, \n"
                    "        routeQuery.`route:date`, \n"
                    "        routeQuery.`route:key`, \n"
                    "        gmOrg.`id` as `organization:id`, \n"
                    "        routeQuery.`route:date` || routeQuery.`route:key` || gmOrg.`id` as `unique_id`, \n"
                    "        COUNT( distinct `routeQuery`.`location:key`) \n"
                    "        FROM as400RouteQuery `routeQuery` \n"
                    "        LEFT JOIN gmOrganizations `gmOrg` \n"
                    "        ON gmOrg.`key` = routeQuery.`organization:key` \n"
                    "        LEFT JOIN "+assignmentTableName+" `dailyAssignment` \n"
                    "        ON `routeQuery`.`route:key` = `dailyAssignment`.`route:key` \n"
                    "        AND `routeQuery`.`route:date` = `dailyAssignment`.`route:date` \n"
                    "        AND `routeQuery`.`organization:key` = `dailyAssignment`.`organization:key` \n"
                    "        LEFT JOIN drivers `mrsDataDrivers` \n"
                    "        ON `dailyAssignment`.`driver:name` = `mrsDataDrivers`.`employeeName` \n"
                    "        LEFT JOIN gmDrivers `gmDriverInfo` \n"
                    "        ON `gmDriverInfo`.`login` = `mrsDataDrivers`.`employeeNumber` \n"
                    "        LEFT JOIN gmEquipment `gmEquipmentInfo` \n"
                    "        ON `gmEquipmentInfo`.`key` = `dailyAssignment`.`truck:key` \n"
                    "        LEFT JOIN routeStartTimes `rst` \n"
                    "        ON `rst`.`route` = `routeQuery`.`route:key` \n"
                    "        LEFT JOIN gmLocations `gmLoc` \n"
                    "        ON `gmLoc`.`key` = `routeQuery`.`organization:key` \n"
                    "        LEFT JOIN gmLocations `gmLocID` \n"
                    "        ON `gmLocID`.`key` = `routeQuery`.`location:key` \n"
                    "        WHERE `routeQuery`.`organization:key` = '"+organizationKey+"' \n"
                    "        AND `routeQuery`.`route:date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "        AND `routeQuery`.`route:key` \n"
                    "        IN \n"
                    "        ( \n"
                    "            SELECT \n"
                    "            `key` \n"
                    "            FROM \n"
                    "            gmRoutes \n"
                    "            WHERE \n"
                    "            `organization:key` = '"+organizationKey+"' \n"
                    "            AND `date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "        ) \n"
                    "        AND `routeQuery`.`route:key` \n"
                    "        IN \n"
                    "        ( \n"
                    "            SELECT \n"
                    "            `route:key` \n"
                    "            FROM "+assignmentTableName+" \n"
                    "            WHERE `organization:key` = '"+organizationKey+"' \n"
                    "            AND `route:date` = '"+date.toString("yyyy-MM-dd")+"' \n"
                    "        ) \n"
                    "        GROUP BY routeQuery.`route:key` \n"
                    "    ) \n"
                    ") \n";

    qDebug() << "Routes out of compliance." << query;
    QMap<QString,QVariantList> sql = executeQuery(query, "Getting route IDs to update.");

    //qDebug() << query << "looking for";
    //qDebug() << sql;
    for(auto var:sql["id"])
    {
        //qDebug() << var;
        returnObj[keyBase + var.toString()] = QJsonValue(QString::number(var.toInt()));
    }
    return returnObj;
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
    //qDebug() << "BridgeDatabase::getAssignmentsToUpdate query " << query;
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
    //qDebug() << returnObj;
    return returnObj;
}

QJsonObject BridgeDatabase::getLocationsToUpdate(const QString &organizationKey)
{
    if(!locationsExist())
        return QJsonObject();

    QJsonObject returnObj;
    QString query = "SELECT \n"
                    "as400LocationQuery.`location:enabled`, \n"
                    "as400LocationQuery.`location:key`, \n"
                    "as400LocationQuery.`location:description`, \n"
                    "as400LocationQuery.`location:addressLine1`, \n"
                    "as400LocationQuery.`location:addressLine2`, \n"
                    "as400LocationQuery.`location:city`, \n"
                    "as400LocationQuery.`location:state`, \n"
                    "as400LocationQuery.`location:zipCode`, \n"
                    "as400LocationQuery.`location:deliveryDays`, \n"
                    "gmLocations.`id` AS `location:id`, \n"
                    "gmOrganizations.`id` AS `organization:id`, \n"
                    "gmAccountTypes.`id` AS `accountType:id`, \n"
                    "as400LocationQuery.`accountType:key`, \n"
                    "gmServiceTimeTypes.`id` AS `serviceTimeType:id`, \n"
                    "as400LocationQuery.`serviceTimeType:key` \n"
                    //"gmLocationTypes.`id` AS `locationType:id`, \n"
                    //"as400LocationQuery.`locationType:key` \n"
                    "FROM as400LocationQuery \n"
                    "JOIN gmLocations \n"
                    "ON gmLocations.`key` = `location:key` \n"
                    "JOIN gmAccountTypes \n"
                    "ON gmAccountTypes.`key` =  as400LocationQuery.`accountType:key` \n"
                    "JOIN gmOrganizations \n"
                    "ON gmOrganizations.`key` = as400LocationQuery.`organization:key` \n"
                    "LEFT JOIN gmServiceTimeTypes \n"
                    "ON gmServiceTimeTypes.`key` = as400LocationQuery.`serviceTimeType:key` \n"
                    "LEFT JOIN gmLocationTypes \n"
                    "ON gmLocationTypes.`key` = as400LocationQuery.`locationType:key` \n"
                    "WHERE gmLocations.`key` \n"
                    "IN \n"
                    "( \n"
                    "    SELECT \n"
                    "    `location:key` \n"
                    "    FROM \n"
                    "    ( \n"
                    "        SELECT DISTINCT \n"
                    "        `location:enabled`, \n"
                    "        `location:key`, \n"
                    "        `location:description`, \n"
                    "        `location:addressLine1`, \n"
                    "        `location:addressLine2`, \n"
                    "        `location:city`, \n"
                    "        `location:state`, \n"
                    "        `location:zipCode`, \n"
                    "        `location:deliveryDays`, \n"
                    "        `accountType:key`, \n"
                    "        `serviceTimeType:key` \n"
                    //"        `locationType:key` \n"
                    "        FROM as400LocationQuery \n"
                    "        WHERE as400LocationQuery.`organization:key` = '"+organizationKey+"' \n"
                    "        EXCEPT \n"
                    "        SELECT DISTINCT \n"
                    "        `enabled`, \n"
                    "        `key`, \n"
                    "        `description`, \n"
                    "        `addressLine1`, \n"
                    "        `addressLine2`, \n"
                    "        `city`, \n"
                    "        `state`, \n"
                    "        `zipCode`, \n"
                    "        `deliveryDays`, \n"
                    "        `accountType:key`, \n"
                    "        `serviceTimeType:key` \n"
                    //"        `locationType:key` \n"
                    "        FROM gmLocations \n"
                    "        WHERE gmLocations.`organization:key` = '"+organizationKey+"' \n"
                    "    ) \n"
                    ") \n";

    qDebug() << "BridgeDatabase::getLocationsToUpdate u wot m8 " <<  query;


    QMap<QString, QVariantList> sql = executeQuery(query, "Getting locations to update in Greenmile. BridgeDatabase::getLocationsToUpdate()");


    if(!sql.empty())
    {
        for(int i = 0; i < sql.first().size(); ++i)
        {
            QString locationKey;
            QStringList locationKeyList;

            QJsonObject locationObj;
            QJsonObject locationTypeObj;
            QJsonObject organizationObj;
            QJsonObject accountTypeObj;
            QJsonObject serviceTimeTypeObj;

            for(auto key:sql.keys())
            {
                QStringList splitKey = key.split(":");
                if(splitKey.first() == "location")
                    locationObj[splitKey.last()] = sql[key][i].toJsonValue();
                if(splitKey.first() == "locationType")
                    locationTypeObj[splitKey.last()] = sql[key][i].toJsonValue();
                if(splitKey.first() == "organization")
                    organizationObj[splitKey.last()] = sql[key][i].toJsonValue();
                if(splitKey.first() == "accountType")
                    accountTypeObj[splitKey.last()] = sql[key][i].toJsonValue();
                if(splitKey.first() == "serviceTimeType")
                    serviceTimeTypeObj[splitKey.last()] = sql[key][i].toJsonValue();
            }

            locationKeyList << "locationToUpdate"
                                << QString::number(locationObj["id"].toInt())
                                << QString::number(organizationObj["id"].toInt());

            locationKey = locationKeyList.join(":");
            locationObj["organization"]     =   QJsonValue(organizationObj);
            locationObj["locationType"]     =   QJsonValue(locationTypeObj);
            locationObj["accountType"]      =   QJsonValue(accountTypeObj);
            locationObj["serviceTimeType"]  =   QJsonValue(serviceTimeTypeObj);
            returnObj[locationKey]          =   QJsonValue(locationObj);
        }
    }
    return returnObj;
}

QJsonObject BridgeDatabase::getLocationsToUpdateGeocodes(const QString &organizationKey)
{
    if(!locationsExist())
        return QJsonObject();

    QJsonObject returnObj;
    QString query = "SELECT `location:enabled`, `location:key`, `location:description`, `location:addressLine1`, `location:addressLine2`, `location:city`, `location:state`, `location:zipCode`, `location:deliveryDays`, `id` AS `location:id`, `locationType:id`, `organization:id` FROM as400LocationQuery LEFT JOIN gmLocations ON `key` = `location:key` WHERE gmLocations.`key` IN ( SELECT `location:key` FROM ( SELECT DISTINCT `location:key`, `location:addressLine1`, `location:addressLine2`, `location:city`, `location:state`, `location:zipCode` FROM as400LocationQuery WHERE as400LocationQuery.`organization:key` = '"+organizationKey+"' EXCEPT SELECT DISTINCT `key`, `addressLine1`,`addressLine2`,`city`, `state`,`zipCode`FROM gmLocations WHERE gmLocations.`organization:key` = '"+organizationKey+"'))";
    QMap<QString, QVariantList> sql = executeQuery(query, "Getting locations geocodes to update in Greenmile. BridgeDatabase::getLocationsToUpdate()");
    qDebug() << "BridgeDatabase::getLocationsToUpdate" <<  query;

    if(!sql.empty())
    {
        for(int i = 0; i < sql.first().size(); ++i)
        {
            QString locationKey;
            QStringList locationKeyList;

            QJsonObject locationObj;
            QJsonObject locationTypeObj;
            QJsonObject organizationObj;

            for(auto key:sql.keys())
            {
                QStringList splitKey = key.split(":");
                if(splitKey.first() == "location")
                    locationObj[splitKey.last()] = sql[key][i].toJsonValue();
                if(splitKey.first() == "locationType")
                    locationTypeObj[splitKey.last()] = sql[key][i].toJsonValue();
                if(splitKey.first() == "organization")
                    organizationObj[splitKey.last()] = sql[key][i].toJsonValue();
            }

            locationKeyList << "locationToUpdate"
                                << QString::number(locationObj["id"].toInt())
                                << QString::number(organizationObj["id"].toInt());

            locationKey = locationKeyList.join(":");
            locationObj["organization"] =   QJsonValue(organizationObj);
            locationObj["locationType"] =   QJsonValue(locationTypeObj);
            returnObj[locationKey]      =   QJsonValue(locationObj);
        }
    }
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

    //qDebug() << "sql empty check";
    //qDebug() << sql;
    bool startsPrevDay = false;
    bool runsBackwards = false;
    if(!sql.empty())
    {
        for(int i = 0; i < sql.first().size(); ++i)
        {
            QJsonObject order;
            QJsonObject location;
            QString routeKey = sql["route:key"][i].toString();
            QString stopKeyTmp = sql["stop:baseLineSequenceNum"][i].toString() + sql["location:id"][i].toString();
            int stopKey = stopKeyTmp.toInt();
            qDebug() << "stopKeyTmp" << stopKeyTmp;


            if(sql["startsPreviousDay"][i].isNull())
            {
                startsPrevDay = false;
            }
            else
            {
                startsPrevDay = true;
            }

            for(auto key:sql.keys())
            {
                splitKey = key.split(":");
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
                            //qDebug() << "start into for route is now";
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
    {
        emit debugMessage("SQL is empty.");
        qDebug() << "sql empty";
    }


    for(auto routeKey:route.keys())
    {
        runsBackwards = false;
        QString dayOfWeek = QDate::fromString(route[routeKey]["date"].toString(), Qt::ISODate).toString("dddd").toLower();
        QString overrideQuery = "SELECT DISTINCT `gmOrigin`.`id` as `origin`, `gmDestination`.`id` as `destination`, CASE WHEN `rteOver`.`"+dayOfWeek+":backwards` = 'FALSE' THEN 0 ELSE 1 END as `backwards` FROM routeOverrides `rteOver` LEFT JOIN gmLocations `gmOrigin` ON `gmOrigin`.`key` = `rteOver`.`"+dayOfWeek+":origin` LEFT JOIN gmLocations `gmDestination` ON `gmDestination`.`key` = `rteOver`.`"+dayOfWeek+":destination` LEFT JOIN gmOrganizations ON gmOrganizations.`key` = `rteOver`.`organization:key` WHERE gmOrganizations.`id` = "+QString::number(organization[routeKey]["id"].toInt())+" AND `rteOver`.`route:key` = '"+routeKey+"'";
        QMap<QString, QVariantList> sql = executeQuery(overrideQuery, "check route overrides.");
        if(!sql.isEmpty())
        {
            for(auto overrideKey:sql.keys())
            {
                if(overrideKey == "backwards")
                {
                    runsBackwards = sql[overrideKey].first().toBool();
                }

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
            qDebug() << "looking for backwards " << route[routeKey]["key"].toString();
            for(auto stopKey:stops[routeKey].keys())
                stopArr.prepend(stops[routeKey][stopKey]);
        }
        else
        {
            qDebug() << "looking for not backwards " << route[routeKey]["key"].toString();
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


        routeCompoundKey        = "routeUpload:" + genericKeyList.join(":");
        driverCompoundKey       = "routeDriverAssignment:" + genericKeyList.join(":");
        equipmentCompoundKey    = "routeEquipmentAssignment:" + genericKeyList.join(":");


        //qDebug() << route[routeKey];
        returnObj[routeCompoundKey] = route[routeKey];
        returnObj[driverCompoundKey] = driver[routeKey];
        returnObj[equipmentCompoundKey] = equipment[routeKey];

        qDebug() << "FInal route upload json" << returnObj;
    }
    //qDebug() << returnObj;
    return returnObj;
}

QJsonObject BridgeDatabase::assembleLocationOverrideTimeWindowFromQuery(const QMap<QString, QVariantList> &sql)
{
    QJsonObject jObj;
    if(isSQLResultValid(sql))
    {
        for(int i = 0; i < sql.first().size(); ++i)
        {
            QString key             = QString();
            QJsonObject lotw        = QJsonObject();
            QJsonObject location    = QJsonObject();

            if(sql["openTime"][i].toTime().isNull() ||
               sql["closeTime"][i].toTime().isNull())
            {
                continue;
            }

            location["id"]      = QJsonValue(sql["location:id"][i].toString());
            lotw["openTime"]    = QJsonValue(sql["openTime"][i].toTime().toString("HH:mm"));
            lotw["closeTime"]   = QJsonValue(sql["closeTime"][i].toTime().toString("HH:mm"));

            if(!sql["tw1Open"][i].isNull())
            {
                lotw["tw1Open"] = QJsonValue(sql["tw1Open"][i].toTime().toString("HH:mm"));
            }
            else
            {
                lotw["tw1Open"] = QJsonValue(sql["openTime"][i].toTime().toString("HH:mm"));
            }

            if(!sql["tw1Close"][i].isNull())
            {
                lotw["tw1Close"]    = QJsonValue(sql["tw1Close"][i].toTime().toString("HH:mm"));
            }
            else
            {
                lotw["tw1Close"] = QJsonValue(sql["closeTime"][i].toTime().toString("HH:mm"));
            }

            if(!sql["tw2Open"][i].isNull())
            {
                lotw["tw2Open"]     = QJsonValue(sql["tw2Open"][i].toTime().toString("HH:mm"));
            }
            if(!sql["tw2Close"][i].isNull())
            {
                lotw["tw2Close"]    = QJsonValue(sql["tw2Close"][i].toTime().toString("HH:mm"));
            }

            lotw["monday"]      = QJsonValue(sql["monday"][i].toBool());
            lotw["tuesday"]     = QJsonValue(sql["tuesday"][i].toBool());
            lotw["wednesday"]   = QJsonValue(sql["wednesday"][i].toBool());
            lotw["thursday"]    = QJsonValue(sql["thursday"][i].toBool());
            lotw["friday"]      = QJsonValue(sql["friday"][i].toBool());
            lotw["saturday"]    = QJsonValue(sql["saturday"][i].toBool());
            lotw["sunday"]      = QJsonValue(sql["sunday"][i].toBool());

            if(sql["id"][i].toString().isEmpty())
            {
                key = "locationOverrideTimeWindowUpload:" + sql["location:key"][i].toString();

            }
            else
            {
                lotw["id"] = QJsonValue::fromVariant(sql["id"][i]);
                key = "locationOverrideTimeWindowUpdate:"
                        + sql["location:key"][i].toString()
                        + ":"
                        + sql["id"][i].toString();
            }

            lotw["location"] = QJsonValue(location);
            jObj[key] = QJsonValue(lotw);
        }
    }
    else
    {
        emit statusMessage("Given empty sql input in BridgeDatabase::assembleLocationOverrideTimeWindowFromQuery. "
                           "Emitting empty result set.");
    }

    return jObj;
}

void BridgeDatabase::SQLDataInsert(const QString &tableName, const QMap<QString, QVariantList> &sql)
{
    if(!okToInsertSQLData(tableName, "SQLDataInsert"))
        return;

    if(!isTableInDB(tableName))
        executeInsertQuery(sqlTableInfoMap_[tableName]["creationQuery"].toString(), QString("create" + tableName));

    writeToTable(tableName, sql);
}

void BridgeDatabase::reprocessAS400LocationTimeWindows()
{
    QString as400LocationTableName = "as400LocationQuery";
    QString selectAllAS400LocationQuery = "SELECT * FROM as400LocationQuery";

    QString as400RouteTableName = "as400RouteQuery";
    QString selectAllAS400RouteQuery = "SELECT * FROM as400RouteQuery";

    QMap<QString, QVariantList> as400LocationResult = executeQuery(selectAllAS400LocationQuery, as400LocationTableName);
    as400LocationResult = fixAS400TimeWindowRecords(as400LocationResult);
    writeToTable("as400LocationQuery", as400LocationResult);

    as400LocationResult.clear();

    as400LocationResult = executeQuery(selectAllAS400RouteQuery, as400RouteTableName);
    as400LocationResult = fixAS400TimeWindowRecords(as400LocationResult);
    writeToTable("as400RouteQuery", as400LocationResult);
}

QMap<QString, QVariantList> BridgeDatabase::fixAS400TimeWindowRecords(const QMap<QString,QVariantList> &records)
{
    QMap<QString,QVariantList> correctedRecords;
    QList<QVariantMap> sqlByRow;
    QVariantMap record;

    if(!isSQLResultValid(records))
        return correctedRecords;

    for(int i = 0; i < records.first().size(); ++i)
    {
        record.clear();
        for(auto key : records.keys())
        {
            record[key] = records[key][i];
        }
        sqlByRow.append(record);
    }

    QtConcurrent::blockingMap(sqlByRow.begin(),sqlByRow.end(), &BridgeDatabase::fixAS400TimeWindowRecord);

    for(auto vMap:sqlByRow)
    {
        for(auto key:vMap.keys())
        {
            correctedRecords[key].append(vMap[key]);
        }
    }

    return correctedRecords;
}

void BridgeDatabase::fixAS400TimeWindowRecord(QVariantMap &record)
{
    bool allInvalid = true;
    QMap<QString, QTime> timeMap;
    QVector<QTime> timeSort;
    timeMap["openTime"]  = record["locationOverrideTimeWindows:openTime"].toTime();
    timeMap["closeTime"] = record["locationOverrideTimeWindows:closeTime"].toTime();
    timeMap["tw1Open"]   = record["locationOverrideTimeWindows:tw1Open"].toTime();
    timeMap["tw1Close"]  = record["locationOverrideTimeWindows:tw1Close"].toTime();
    timeMap["tw2Open"]   = record["locationOverrideTimeWindows:tw2Open"].toTime();
    timeMap["tw2Close"]  = record["locationOverrideTimeWindows:tw2Close"].toTime();

    //Validate Pairs
    if(!timeMap["openTime"].isValid() || !timeMap["closeTime"].isValid())
    {
        timeMap["openTime"] = QTime();
        timeMap["closeTime"] = QTime();
    }

    if(!timeMap["tw1Open"].isValid() || !timeMap["tw1Close"].isValid())
    {
        timeMap["tw1Open"] = QTime();
        timeMap["tw1Close"] = QTime();
    }

    if(!timeMap["tw2Open"].isValid() || !timeMap["tw2Close"].isValid())
    {
        timeMap["tw2Open"] = QTime();
        timeMap["tw2Close"] = QTime();
    }

    //Overall validity.
    for(auto time:timeMap.values())
    {
        if(time.isValid())
        {
            allInvalid = false;
        }
        if(time == QTime(0,1))
        {
            time = QTime(0,0);
        }
    }


    if(!allInvalid && !timeMap["openTime"].isValid() && !timeMap["closeTime"].isValid())
    {
        //qDebug() << "perform time shuffle";
        if(timeMap["tw1Open"].isValid() && timeMap["tw1Close"].isValid())
        {
            //qDebug() << "shuffle 1 to oc";
            timeMap["openTime"] = timeMap["tw1Open"];
            timeMap["closeTime"] = timeMap["tw1Close"];
            timeMap["tw1Open"]  = QTime();
            timeMap["tw1Close"] = QTime();

            if(timeMap["tw2Open"].isValid() && timeMap["tw2Close"].isValid())
            {
                //qDebug() << "shuffle 2 to 1";
                timeMap["tw1Open"] = timeMap["tw2Open"];
                timeMap["tw1Close"] = timeMap["tw2Close"];
                timeMap["tw2Open"]  = QTime();
                timeMap["tw2Close"] = QTime();
            }
        }
        else if(timeMap["tw2Open"].isValid() && timeMap["tw2Close"].isValid())
        {
            //qDebug() << "shuffle 2 to oc";
            //qDebug() << timeMap["tw1Open"] << timeMap["tw1Close"];
            //qDebug() << record["location:key"];
            timeMap["openTime"] = timeMap["tw2Open"];
            timeMap["closeTime"] = timeMap["tw2Close"];
            timeMap["tw2Open"] = QTime();
            timeMap["tw2Close"] = QTime();
        }
    }

    //Split times over the date line.
    if(timeMap["openTime"] > timeMap["closeTime"])
    {
        //qDebug() << "crosses over midnight.";
        timeMap["tw1Open"] = timeMap["openTime"];
        timeMap["tw1Close"] = QTime(23,58);
        timeMap["tw2Open"] = QTime(0, 1);
        timeMap["tw2Close"] = timeMap["closeTime"];
        timeMap["openTime"] = QTime(0,0);
        timeMap["closeTime"] = QTime(23,59);
    }

    for(auto key:timeMap.keys())
    {
        if(timeMap[key].isValid())
        {
            timeSort.append(timeMap[key]);
        }
    }

    std::sort(timeSort.begin(), timeSort.end());

    //Adjusts time windows so that they do no overlap.
    //Special consideration to not push a TW beyond the date e.g. 23:59 -> 00:00.
    if(record["location:key"].toString() == "61565")
    {
        qDebug() << timeSort;
    }

    for(int i = 0; i < timeSort.size()-1; ++i)
    {
        while(timeSort[i] >= timeSort[i+1] && timeSort[i+1].addSecs(60) != QTime(0,0))
        {
            timeSort[i+1] = timeSort[i+1].addSecs(60);
        }
        while(timeSort[i] >= timeSort[i+1] && timeSort[i].addSecs(-60) != QTime(23,59))
        {
            timeSort[i] = timeSort[i].addSecs(-60);
        }
        if(i > 1 && timeSort[i] == timeSort[i-1] && timeSort[i-1].addSecs(-60) != QTime(23,59))
        {
            timeSort[i-1] = timeSort[i-1].addSecs(-60);
        }
    }

    std::sort(timeSort.begin(), timeSort.end());

    if(record["location:key"].toString() == "61565")
    {
        qDebug() << timeSort;
    }

    for(int i = 0; i < timeSort.size(); ++i)
    {
        int last = timeSort.size()-1;
        if(i != last)
        {
            switch(i)
            {
            case 0:
                timeMap["openTime"] = timeSort[i];
                break;
            case 1:
                timeMap["tw1Open"] = timeSort[i];
                break;
            case 2:
                timeMap["tw1Close"] = timeSort[i];
                break;
            case 3:
                timeMap["tw2Open"] = timeSort[i];
                break;
            case 4:
                timeMap["tw2Close"] = timeSort[i];
                break;
            }
        }
        else
        {
            timeMap["closeTime"] = timeSort[i];
        }
    }

    for(auto key:timeMap.keys())
    {
        record["locationOverrideTimeWindows:" + key] = QVariant(timeMap[key]);
    }
}


bool BridgeDatabase::isSQLResultValid(const QMap<QString, QVariantList> &data)
{
    int columnSizeCount;
    int otherColumnSizeCount;

    if(data.isEmpty())
    {
        emit debugMessage("SQL result is invalid.");
        return false;
    }

    if(data.first().isEmpty())
    {
        emit debugMessage("SQL result is empty.");
        return true;
    }

    columnSizeCount = data.first().size();
    //qDebug() << columnSizeCount;
    for(auto key:data.keys())
    {
        otherColumnSizeCount = data[key].size();
        //qDebug() << key;
        //qDebug() << otherColumnSizeCount;
        if(columnSizeCount != otherColumnSizeCount)
        {
            emit errorMessage("ERROR: SQL columns are not all of equal length. The last column compared was " + key);
            return false;
        }
    }

    return true;
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
        //qDebug() << whatMethod + ": JSON array info cannot be null.";
        return ok;
    }
    if(jsonTableInfoMap_[tableName]["creationQuery"].toString().isNull() || jsonTableInfoMap_[tableName]["creationQuery"].toString().isEmpty())
    {
        emit errorMessage(whatMethod + ": JSON array table creation query cannot be null");
        //qDebug() << whatMethod + ": JSON array table creation query cannot be null";
        return ok;
    }
    if(jsonTableInfoMap_[tableName].isEmpty())
    {
        emit errorMessage(whatMethod + ": Expected JSON object keys cannot be null.");
        //qDebug() << whatMethod + ": Expected JSON object keys cannot be null.";
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
        //qDebug() << whatMethod + ": SQL table info cannot be null.";
        return ok;
    }
    if(sqlTableInfoMap_[tableName]["creationQuery"].toString().isNull() || sqlTableInfoMap_[tableName]["creationQuery"].toString().isEmpty())
    {
        emit errorMessage(whatMethod + ": SQL table creation query cannot be null");
        //qDebug() << whatMethod + ": SQL table creation query cannot be null";
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
        //qDebug() << "JSON array info cannot be null.";
        return ok;
    }
    if(tableCreationQuery.isNull() || tableCreationQuery.isEmpty())
    {
        emit errorMessage("JSON array table creation query cannot be null");
        //qDebug() << "JSON array table creation query cannot be null";
        return ok;
    }
    if(expectedJsonKeys.isEmpty())
    {
        emit errorMessage("Expected JSON object keys cannot be null.");
        //qDebug() << "Expected JSON object keys cannot be null.";
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
        //qDebug() << "SQL table name info cannot be null.";
        return ok;
    }
    if(tableCreationQuery.isNull() || tableCreationQuery.isEmpty())
    {
        emit errorMessage("SQL table creation query cannot be null");
        //qDebug() << "SQL table creation query cannot be null";
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
    //    //qDebug() << vMap.size();
    //    for(auto key:vMap.keys())
    //        //qDebug() << key << vMap[key].type() << vMap["id"];
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
        return QVariant(QString(arrayToString.toJson(QJsonDocument::Compact)));
    }

    case QJsonValue::Object:
    {
        QJsonDocument objToString;
        objToString.setObject(val.toObject());
        return QVariant(QString(objToString.toJson(QJsonDocument::Compact)));
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
    return executeInsertQuery(truncateTableQuery, "Truncating " + tableName + ".");
}

bool BridgeDatabase::writeToTable(const QString &tableName, QMap<QString, QVariantList> data)
{
    bool success = false;

    if(!isSQLResultValid(data))
        return success;

    qDebug() << data.keys() << "1";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbPath_);
        db.setDatabaseName(dbPath_);

        emit statusMessage("SQLite " + tableName + " import.");
        if(db.open())
        {
            success = executeQueryAsString(db,tableName, data);
            qDebug() << data.keys() << "2";
            if(!success)
            {
                success = executeQueryResiliantly(db,tableName, data);
                qDebug() << data.keys() << "3";
            }
        }
        else
        {
            emit errorMessage("Failed to open SQLite database.");
            emit errorMessage(db.lastError().text());
        }

        db.close();
        emit statusMessage("Finished SQLite. INSERT OR REPLACE for " + tableName);

    }
    emit debugMessage("Cleaning up SQLite " + dbPath_ + " connection.");
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

        emit statusMessage("Beginning SQLite query to " +verb);
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
        emit debugMessage("Finished SQLite. " + verb);
    }
    emit debugMessage("Cleaning up SQLite " + dbPath_ + " connection.");
    QSqlDatabase::removeDatabase(dbPath_);

    return sqlData;
}

bool BridgeDatabase::executeASYNCQuery(const QString &queryString, const QString &queryKey, const int chunkSize, const QString &verb)
{
    bool success = false;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbPath_);
        db.setDatabaseName(dbPath_);

        emit statusMessage("Beginning SQLite " + verb);
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
        emit statusMessage("Finished SQLite. "+verb);
    }
    emit debugMessage("Cleaning up SQLite " + dbPath_ + " connection.");
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
                //qDebug() << verb + ": SQLite query returned an empty result set.";
            }
//            else
//                emit statusMessage(QString(verb + ": Retrieved " +  QString::number(sqlData.first().size()) + " records from SQLite."));

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
        //qDebug() << verb + ": SQLite query returned an empty result set.";
    }
//    else
//        emit statusMessage(QString(verb + ": Retrieved " +  QString::number(sqlData.first().size()) + " records from SQLite."));

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
        emit statusMessage("Finished SQLite. "+verb);
    }
    emit debugMessage("Cleaning up SQLite " + dbPath_ + " connection.");
    QSqlDatabase::removeDatabase(dbPath_);
    return success;
}

QStringList BridgeDatabase::generateValueTuples(QMap<QString, QVariantList> data)
{
    QStringList valueList;
    QStringList valueTuples;
    for(int i = 0; i < data.first().size(); ++i)
    {
        valueList.clear();
        for(auto key:data.keys())
        {
            if(data[key].size() > data.first().size())
            {
                emit errorMessage("Cannot assemble value tuples. Index for " + key + " out of range.");
                return QStringList();
            }
            switch(data[key][i].type()) {

            case QVariant::Type::Bool:
                if(data[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(data[key][i].toString());
                break;

            case QVariant::Type::Int:
                if(data[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(data[key][i].toString());
                break;

            case QVariant::Type::LongLong:
                if(data[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(data[key][i].toString());
                break;

            case QVariant::Type::Double:
                if(data[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append(data[key][i].toString());
                break;

            case QVariant::Type::String:
                if(data[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + data[key][i].toString().toLatin1().replace("\"", "\"\"") + "\"");
                break;

            case QVariant::Type::Date:
                if(data[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + data[key][i].toDate().toString(Qt::ISODate) + "\"");
                break;

            case QVariant::Type::DateTime:
                if(data[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + data[key][i].toDate().toString(Qt::ISODate) + "\"");
                break;

            case QVariant::Type::Time:
                if(data[key][i].isNull())
                    valueList.append("NULL");
                else
                    valueList.append("\"" + data[key][i].toTime().toString(Qt::ISODate) + "\"");
                break;

            case QVariant::Type::Invalid:
                valueList.append("NULL");
                break;

            default:
                qApp->processEvents();
                //qDebug() << "Unknown variant type in switch. "
                //            "If you see this a lot, "
                //            " your event loop might be getting smashed... " << data[key][i].type();
                emit errorMessage(QString("Unsupported data type from SQLite database "
                                          + data[key][i].toString()
                                          + " "
                                          + QString(data[key][i].type())));
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
    //qDebug() << "batch insert";
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
    emit debugMessage(QString("Query length for " + tableName + " is " + QString::number(queryString.size()) + " char."));
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
            //qDebug() << "Failed on query "  + queryString;
        }

        db.driver()->commitTransaction();
    }

    emit statusMessage("Finished resiliant INSERT for " + tableName);

    return success;
}
