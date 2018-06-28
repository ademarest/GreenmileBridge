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
              "SELECT orderHeaders.HHHRTEN AS \"route:key\", "
              "orderHeaders.HHHDTEI AS \"route:date\", "
              "orderHeaders.HHHCUSN AS \"location:key\", "
              "orderHeaders.HHHCNMB AS \"location:description\", "
              "orderHeaders.HHHCA1B AS \"location:addressLine1\", "
              "orderHeaders.HHHCA2B AS \"location:addressLine2\", "
              "orderHeaders.HHHCTYB AS \"location:city\", "
              "orderHeaders.HHHSTEB AS \"location:state\", "
              "orderHeaders.HHHZPCB AS \"location:zipCode\", "
              "orderHeaders.HHHSTPN AS \"stop:plannedSequenceNum\", "
              "companyInfo.L1NAME AS \"organization:key\", "
              "sizeInfo.\"stop:pieces\" AS \"stop:pieces\", "
              "sizeInfo.\"stop:weight\" AS \"stop:weight\", "
              "sizeInfo.\"stop:cube\" AS \"stop:cube\", "
              "assignmentInfo.EERDRVN AS \"driver:key\", "
              "assignmentInfo.EERTKNB AS \"equipment:key\" "
              "FROM PWRDTA.HHHORDHL6 AS orderHeaders "
              "INNER JOIN PWRUSER.CMPNLIST1 AS companyInfo "
              "ON companyInfo.L1LOC = TRIM(orderHeaders.HHHCMPN) || TRIM(orderHeaders.HHHDIVN) || TRIM(orderHeaders.HHHDPTN) "
              "INNER JOIN(SELECT HHHCUSF AS \"as400:masterStopNumber\", SUM(HHHQYSA) AS \"stop:pieces\", SUM(HHHEXSH) AS \"stop:weight\", SUM(HHHEXCB) AS \"stop:cube\" FROM PWRDTA.HHHORDHL6 WHERE HHHDTEI = " + date.toString("yyyyMMdd") +" GROUP BY HHHCUSF) AS sizeInfo "
              "ON sizeInfo.\"as400:masterStopNumber\" = orderHeaders.HHHCUSF "
              "INNER JOIN PWRDTA.EERRTMAL0 AS assignmentInfo "
              "ON assignmentInfo.EERRTEN = orderHeaders.HHHRTEN "
              "WHERE orderHeaders.HHHDTEI = " + date.toString("yyyyMMdd") + " "
              "AND orderHeaders.HHHCUSF = orderHeaders.HHHCUSN "
              "AND TRIM(assignmentInfo.EERCMPN) || TRIM(assignmentInfo.EERDIVN) || TRIM(assignmentInfo.EERDPTN) = TRIM(orderHeaders.HHHCMPN) || TRIM(orderHeaders.HHHDIVN) || TRIM(orderHeaders.HHHDPTN) "
              "AND orderHeaders.HHHCRIN<>'Y' "
              "GROUP BY orderHeaders.HHHRTEN, "
              "orderHeaders.HHHDTEI, "
              "orderHeaders.HHHCUSF, "
              "orderHeaders.HHHCUSN, "
              "orderHeaders.HHHCNMB, "
              "orderHeaders.HHHCA1B, "
              "orderHeaders.HHHCA2B, "
              "orderHeaders.HHHCTYB, "
              "orderHeaders.HHHSTEB, "
              "orderHeaders.HHHZPCB, "
              "orderHeaders.HHHSTPN, "
              "companyInfo.L1NAME, "
              "sizeInfo.\"stop:pieces\", "
              "sizeInfo.\"stop:weight\", "
              "sizeInfo.\"stop:cube\", "
              "assignmentInfo.EERDRVN, "
              "assignmentInfo.EERTKNB";

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
