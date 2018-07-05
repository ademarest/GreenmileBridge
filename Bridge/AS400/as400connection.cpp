#include "as400connection.h"

AS400::AS400(QObject *parent) : QObject(parent)
{
    jsonSettings_ = settings_.loadSettings(QFile(dbPath_), jsonSettings_);
}

AS400::AS400(const QString &systemIP, const QString &username, const QString &password, QObject *parent) : QObject(parent)
{
    jsonSettings_["driver"] = "iSeries Access ODBC Driver";
    jsonSettings_["system"] = systemIP;
    jsonSettings_["username"] = username;
    jsonSettings_["password"] = password;
    settings_.saveSettings(QFile(dbPath_), jsonSettings_);
}

void AS400::init()
{
    connect(&settings_, &JsonSettings::debugMessage, this, &AS400::debugMessage);
}

bool AS400::getInvoiceData(const QDate &minDate, const QDate &maxDate, const int chunkSize)
{
    QString queryString("SELECT invn AS \"invoiceNumber\","
                        "whsn AS \"warehouseNumber\", "
                        "rten AS \"route\", "
                        "stpn AS \"stopNumber\", "
                        "crin AS \"credit\", "
                        "dtei AS \"invoiceDate\", "
                        "dtes AS \"shipDate\", "
                        "dtet AS \"orderDate\", "
                        "timo AS \"orderTime\", "
                        "exsh AS \"weight\", "
                        "excb AS \"caseCube\", "
                        "qysa AS \"casesShipped\", "
                        "qyoa AS \"casesOrdered\", "
                        "exsn AS \"netSales\", "
                        "exac AS \"productCost\", "
                        "exgp AS \"profit\", "
                        "ppft AS \"profitPercent\", "
                        "cusn AS \"customerNumber\", "
                        "slnb AS \"salesRep\", "
                        "demp AS \"driverNumber\", "
                        "tknb AS \"truckNumber\"  "
                        "FROM pwruser.sqlinvhdl0 "
                        "WHERE pday "
                        "BETWEEN \'"
                        +minDate.toString("yyyy-MM-dd")
                        +"\' AND \'"
                        +maxDate.toString("yyyy-MM-dd")+"\'");

    return queryAS400(AS400QueryType::Invoice, queryString, chunkSize);
}


bool AS400::getCustomerChains(const int chunkSize)
{
    QString queryString = "SELECT CAST(BBSCMPN AS INT) AS \"companyNumber\", "
                          "CAST(BBSDIVN AS INT) AS \"divisionNumber\", "
                          "CAST(BBSDPTN AS INT) AS \"departmentNumber\", "
                          "REPLACE(BBSCSCD,' ','') AS \"chainStoreCode\", "
                          "BBSCTDC AS \"chainDescription\" "
                          "FROM PWRDTA.BBSCHNHP";

    return queryAS400(AS400QueryType::CustomerChain, queryString,chunkSize);
}

bool AS400::getOpenOrderHeaders(const int chunkSize)
{
    QString queryString = "SELECT oohcusn AS \"customerNumber\","
                          "oohornr AS \"orderNumber\","
                          "oohmemo AS \"memoCode\","
                          "oohslnb AS \"salesRep\","
                          "oohdtes AS \"shipDate\","
                          "oohdtet AS \"orderDate\","
                          "oohrten AS \"route\","
                          "oohstpn AS \"stop\","
                          "oohinsi AS \"specialInstructions1\","
                          "oohins2 AS \"specialInstructions2\","
                          "oohpono AS \"customerPO\","
                          "oohoalc AS \"allocatedFlag\","
                          "oohpkpr AS \"pickTicketPrinted\","
                          "oohinam AS \"totalInvoiceAmount\","
                          "oohexac AS \"actualAmount\","
                          "oohexcb AS \"caseCube\","
                          "oohexsh AS \"shipWeight\","
                          "oohqyoa AS \"totalQtyOrdered\","
                          "oohqysa AS \"totalQtyShipped\" "
                          "FROM pwrdta.OOHORDHPS";

    return queryAS400(AS400QueryType::OpenOrderHeader, queryString, chunkSize);
}

bool AS400::getOpenOrderDetails(const int chunkSize)
{
    QString queryString = "Select ooiornr AS \"orderNumber\","
                          "CAST(replace(ooiitmn,'-','') AS BIGINT) AS \"itemNumber\","
                          "ooiqyoa AS \"qtyOrdered\","
                          "ooiqysa AS \"qtyShipped\","
                          "ooiwhna AS \"warehouseArea\","
                          "ooiisle AS \"aisle\","
                          "ooisltn AS \"slot\","
                          "ooibuyn AS \"buyerNumber\","
                          "ooivndn AS \"vendorNumber\","
                          "ooisbcc AS \"substituteFlag\","
                          "ooiitmo As \"itemNumberBeforeSub\","
                          "ooiuwnr AS \"uow\" "
                          "FROM pwrdta.OOIORDDPS";

    return queryAS400(AS400QueryType::OpenOrderDetail, queryString, chunkSize);
}

bool AS400::getCustomerData()
{
    bool success = false;
    return success;
}

bool AS400::getRouteDataForGreenmile(const QDate &date, const int chunkSize)
{
    QString queryString =
              "SELECT TRIM(orderInfo.HHHCUSF) AS \"location:key\", "
              "TRIM(orderInfo.HHHRTEN) AS \"route:key\", "
              "DATE(TIMESTAMP_FORMAT(CHAR(orderInfo.HHHDTEI), 'YYYY-MM-DD')) AS \"route:date\", "
              "TRIM(orderInfo.HHHCUSN) || ' - ' || TRIM(orderInfo.HHHCNMB) || ' - ' || TRIM(orderInfo.HHHINVN) AS \"order:number\", "
              "orderInfo.HHHQYSA AS \"order:pieces\", "
              "orderInfo.HHHEXSH AS \"order:weight\", "
              "orderInfo.HHHEXCB AS \"order:cube\", "
              "TRIM(companyInfo.L1NAME) AS \"organization:key\", "
              "stopInfo.\"stop:baseLineSequenceNum\" AS \"stop:plannedSequenceNumber\", "
              "TRIM(assignmentInfo.EERDRVN) AS \"driver:key\", "
              "TRIM(assignmentInfo.EERTKNB) AS \"equipment:key\", "
              "TRIM(customerInfo.FFDCNMB) AS \"location:description\", "
              "TRIM(customerInfo.FFDCA1B) AS \"location:addressLine1\", "
              "TRIM(customerInfo.FFDCA2B) AS \"location:addressLine2\", "
              "TRIM(customerInfo.FFDCTYB) AS \"location:city\", "
              "TRIM(customerInfo.FFDSTEB) AS \"location:state\", "
              "TRIM(customerInfo.FFDZPCB) AS \"location:zipCode\", "
              "CASE WHEN customerWindows.JJCTSR1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Open\", "
              "CASE WHEN customerWindows.JJCTSP1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Close\", "
              "CASE WHEN customerWindows.JJCTSR2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Open\", "
              "CASE WHEN customerWindows.JJCTSP2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Close\", "
              "CASE WHEN customerWindows.JJCOPEN = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCOPEN),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:openTime\", "
              "CASE WHEN customerWindows.JJCCLOS = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCCLOS),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:closeTime\", "
              "TRIM(LEADING ',' FROM deliveryDays.\"location:deliveryDays\") AS \"location:deliveryDays\" "
              "FROM PWRDTA.HHHORDHL6 AS orderInfo "
              "INNER JOIN PWRUSER.CMPNLIST1 AS companyInfo "
              "ON companyInfo.L1LOC = TRIM(orderInfo.HHHCMPN) || TRIM(orderInfo.HHHDIVN) || TRIM (orderInfo.HHHDPTN) "
              "INNER JOIN (SELECT HHHCUSF AS \"location:key\", MAX(HHHSTPN) AS \"stop:baseLineSequenceNum\" FROM PWRDTA.HHHORDHL6 WHERE HHHDTEI = "+date.toString("yyyyMMdd")+" AND HHHCUSF = HHHCUSN GROUP BY HHHCUSF) AS stopInfo "
              "ON TRIM(stopInfo.\"location:key\") = TRIM(orderInfo.HHHCUSF) "
              "INNER JOIN PWRDTA.EETRTEAP AS customerRouting "
              "ON TRIM(customerRouting.EETCUSN) = TRIM(orderInfo.HHHCUSF) "
              "INNER JOIN PWRDTA.EERRTMAL0 AS assignmentInfo "
              "ON TRIM(orderInfo.HHHRTEN) || TRIM(orderInfo.HHHCMPN) || TRIM(orderInfo.HHHDIVN) || TRIM (orderInfo.HHHDPTN) = TRIM(assignmentInfo.EERRTEN) || TRIM(assignmentInfo.EERCMPN) || TRIM(assignmentInfo.EERDIVN) || TRIM (assignmentInfo.EERDPTN) "
              "INNER JOIN PWRDTA.FFDCSTBL0 AS customerInfo "
              "ON TRIM(customerInfo.FFDCUSN) = TRIM(orderInfo.HHHCUSF) "
              "INNER JOIN PWRDTA.JJCCSTRL0 AS customerWindows "
              "ON TRIM(customerWindows.JJCCUSN) = TRIM(orderInfo.HHHCUSF) "
              "INNER JOIN (SELECT CASE WHEN LENGTH(TRIM(EETRTF1))>0 THEN ',M' ELSE '' END "
              "|| CASE WHEN LENGTH(TRIM(EETRTF2))>0 THEN ',T' ELSE '' END "
              "|| CASE WHEN LENGTH(TRIM(EETRTF3))>0 THEN ',W' ELSE '' END "
              "|| CASE WHEN LENGTH(TRIM(EETRTF4))>0 THEN ',R' ELSE '' END "
              "|| CASE WHEN LENGTH(TRIM(EETRTF5))>0 THEN ',F' ELSE '' END "
              "|| CASE WHEN LENGTH(TRIM(EETRTF6))>0 THEN ',S' ELSE '' END "
              "|| CASE WHEN LENGTH(TRIM(EETRTF7))>0 THEN ',U' ELSE '' END AS \"location:deliveryDays\", "
              "EETCUSN AS customerNumber FROM PWRDTA.EETRTEAL0) AS deliveryDays "
              "ON TRIM(deliveryDays.customerNumber) = TRIM(orderInfo.HHHCUSF) "
              "WHERE orderInfo.HHHDTEI = "+date.toString("yyyyMMdd")+" AND orderInfo.HHHCRIN<>'Y'";

    return queryAS400(AS400QueryType::GreenmileRouteInfo, queryString, chunkSize);
}

bool AS400::queryAS400(const AS400QueryType queryType, const QString &queryString, const int chunkSize)
{
    bool success = false;
    QString connectString = "DRIVER={"
            + jsonSettings_["driver"].toString()
            + "};SYSTEM="
            + jsonSettings_["system"].toString()
            + ";";
    emit debugMessage(connectString);
    {
        QSqlDatabase odbc = QSqlDatabase::addDatabase("QODBC", "AS400");
        odbc.setUserName(jsonSettings_["username"].toString());
        odbc.setPassword(jsonSettings_["password"].toString());
        odbc.setDatabaseName(connectString);

        if(odbc.open())
        {
            QSqlQuery query(odbc);

            emit debugMessage(QString("Running against AS400 with a chunk size of " + QString::number(chunkSize)));
            emit debugMessage(QString("If you have an overflow, reduce chunk size."));
            emit debugMessage(queryString);

            if(query.exec(queryString))
            {
                success = true;
                processQuery(queryType, query, chunkSize);
            }
            else
            {
                success = false;
                emit debugMessage("AS400 Query Error: " + query.lastError().text());
            }

        }
        else
        {
            emit debugMessage("There was an error with AS400.");
            emit debugMessage(odbc.lastError().text());
            success = false;
        }
    }
    emit debugMessage("Cleaning up AS400 connection.");
    QSqlDatabase::removeDatabase("AS400");
    return success;
}

void AS400::processQuery(const AS400QueryType queryType, QSqlQuery &query,const int chunkSize)
{
    bool firstRun = true;
    int recordCounter = 0;
    QMap<QString,QVariantList> sqlData;
    while(query.next())
    {
        if(recordCounter == chunkSize)
        {
            if(sqlData.isEmpty())
            {
                emit debugMessage("AS400 query returned an empty result set - not emitting result set.");
                return;
            }

            //Count the amt of records. Explodes if chunk size is zero, etc.
            emit debugMessage(QString("Retrieved " +  QString::number(sqlData.first().size()) + " records from AS400."));

            dispatchSqlResults(firstRun, queryType, sqlData);
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
    emit debugMessage(QString("Retrieved " +  QString::number(sqlData.first().size()) + " records from AS400."));

    if(sqlData.isEmpty())
    {
        emit debugMessage("AS400 query returned an empty result set - not emitting result set.");
        return;
    }

    dispatchSqlResults(firstRun, queryType, sqlData);

    for(auto key: sqlData.keys())
        sqlData[key].clear();

    sqlData.clear();
    return;
}

void AS400::dispatchSqlResults(const bool isFirstRun,
                               const AS400QueryType queryType,
                               const QMap<QString, QVariantList> &sqlResults)
{
    switch (queryType)
    {
    case AS400QueryType::GreenmileRouteInfo :
        emit greenmileRouteInfoResults(sqlResults);
        break;

    case AS400QueryType::Invoice :
        emit invoiceDataResults(sqlResults);
        break;

    case AS400QueryType::CustomerChain :
        emit customerChainResults(sqlResults);
        break;

    case AS400QueryType::OpenOrderHeader :
        emit openOrderHeaderResults(isFirstRun, sqlResults);
        break;

    case AS400QueryType::OpenOrderDetail :
        emit openOrderDetailResults(isFirstRun, sqlResults);
        break;
    }
}
