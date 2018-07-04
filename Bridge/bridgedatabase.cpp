#include "bridgedatabase.h"

BridgeDatabase::BridgeDatabase(QObject *parent) : QObject(parent)
{

}

void BridgeDatabase::makeAS400RouteQueryTable()
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

}
