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

bool AS400::getInvoiceData(const QString &key, const QDate &minDate, const QDate &maxDate, const int chunkSize)
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

    return queryAS400(key, queryString, chunkSize);
}


bool AS400::getCustomerChains(const QString &key, const int chunkSize)
{
    QString queryString = "SELECT CAST(BBSCMPN AS INT) AS \"companyNumber\", "
                          "CAST(BBSDIVN AS INT) AS \"divisionNumber\", "
                          "CAST(BBSDPTN AS INT) AS \"departmentNumber\", "
                          "REPLACE(BBSCSCD,' ','') AS \"chainStoreCode\", "
                          "BBSCTDC AS \"chainDescription\" "
                          "FROM PWRDTA.BBSCHNHP";

    return queryAS400(key, queryString,chunkSize);
}

bool AS400::getOpenOrderHeaders(const QString &key, const int chunkSize)
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

    return queryAS400(key, queryString, chunkSize);
}

bool AS400::getOpenOrderDetails(const QString &key, const int chunkSize)
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

    return queryAS400(key, queryString, chunkSize);
}

bool AS400::getCustomerData(const QString &key)
{
    bool success = false;
    qDebug() << "Customer data under key " << key << "not implemented yet.";
    return success;
}

bool AS400::getRouteDataForGreenmile(const QString &key, const QDate &date, const int chunkSize)
{
    QString queryString = "SELECT DISTINCT TRIM(companyInfo.L1NAME) AS \"organization:key\", TRIM(orderInfo.HHHRTEN) AS \"route:key\", stopInfo.\"stop:baseLineSequenceNum\" AS \"stop:baseLineSequenceNum\", DATE(TIMESTAMP_FORMAT(CHAR(orderInfo.HHHDTEI), 'YYYY-MM-DD')) AS \"route:date\", TRIM(orderInfo.HHHCUSN) || ' - ' || TRIM(orderInfo.HHHCNMB) || ' - ' || TRIM(orderInfo.HHHINVN) AS \"order:number\", orderInfo.HHHQYSA AS \"order:pieces\", orderInfo.HHHEXSH AS \"order:weight\", orderInfo.HHHEXCB AS \"order:cube\", TRIM(orderInfo.HHHCUSF) AS \"location:key\", CASE WHEN TRIM(customerInfo.FFDCNMB) = '' THEN NULL ELSE TRIM(customerInfo.FFDCNMB) END AS \"location:description\", CASE WHEN TRIM(customerInfo.FFDCA1B) = '' THEN NULL ELSE TRIM(customerInfo.FFDCA1B) END AS \"location:addressLine1\", CASE WHEN TRIM(customerInfo.FFDCA2B) = '' THEN NULL ELSE TRIM(customerInfo.FFDCA2B) END AS \"location:addressLine2\", CASE WHEN TRIM(customerInfo.FFDCTYB) = '' THEN NULL ELSE TRIM(customerInfo.FFDCTYB) END AS \"location:city\", CASE WHEN TRIM(customerInfo.FFDSTEB) = '' THEN NULL ELSE TRIM(customerInfo.FFDSTEB) END AS \"location:state\", CASE WHEN TRIM(customerInfo.FFDZPCB) = '' THEN NULL ELSE TRIM(customerInfo.FFDZPCB) END AS \"location:zipCode\", CASE WHEN customerWindows.JJCTSR1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Open\", CASE WHEN customerWindows.JJCTSP1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Close\", CASE WHEN customerWindows.JJCTSR2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Open\", CASE WHEN customerWindows.JJCTSP2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Close\", CASE WHEN customerWindows.JJCOPEN = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCOPEN),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:openTime\", CASE WHEN customerWindows.JJCCLOS = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCCLOS),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:closeTime\", CASE WHEN TRIM(LEADING ',' FROM deliveryDays.\"location:deliveryDays\") = '' THEN NULL ELSE TRIM(LEADING ',' FROM deliveryDays.\"location:deliveryDays\") END AS \"location:deliveryDays\", CASE WHEN TRIM(assignmentInfo.EERDRVN) = '' THEN NULL ELSE TRIM(assignmentInfo.EERDRVN) END AS \"driver:key\", CASE WHEN TRIM(assignmentInfo.EERTKNB) = '' THEN NULL ELSE TRIM(assignmentInfo.EERTKNB) END AS \"equipment:key\" FROM PWRDTA.HHHORDHL6 AS orderInfo INNER JOIN PWRUSER.CMPNLIST1 AS companyInfo ON companyInfo.L1LOC = TRIM(orderInfo.HHHCMPN) || TRIM(orderInfo.HHHDIVN) || TRIM (orderInfo.HHHDPTN) INNER JOIN (SELECT HHHCUSF AS \"location:key\",HHHRTEN AS \"route:key\",HHHDTEI AS \"route:date\",MAX(HHHSTPN) AS \"stop:baseLineSequenceNum\" FROM PWRDTA.HHHORDHL6 WHERE HHHDTEI = "+date.toString("yyyyMMdd")+" GROUP BY HHHCUSF, HHHRTEN, HHHDTEI) AS stopInfo ON TRIM(stopInfo.\"location:key\") = TRIM(orderInfo.HHHCUSF) AND TRIM(stopInfo.\"route:key\") = TRIM(orderInfo.HHHRTEN) AND TRIM(stopInfo.\"route:date\") = TRIM(orderInfo.HHHDTEI) INNER JOIN PWRDTA.FFDCSTBL0 AS customerInfo ON TRIM(customerInfo.FFDCUSN) = TRIM(orderInfo.HHHCUSF) LEFT JOIN PWRDTA.EERRTMAL0 AS assignmentInfo ON TRIM(orderInfo.HHHRTEN) || TRIM(orderInfo.HHHCMPN) || TRIM(orderInfo.HHHDIVN) || TRIM (orderInfo.HHHDPTN) = TRIM(assignmentInfo.EERRTEN) || TRIM(assignmentInfo.EERCMPN) || TRIM(assignmentInfo.EERDIVN) || TRIM (assignmentInfo.EERDPTN) LEFT JOIN (SELECT CASE WHEN LENGTH(TRIM(EETRTF1))>0 THEN ',M' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF2))>0 THEN ',T' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF3))>0 THEN ',W' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF4))>0 THEN ',R' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF5))>0 THEN ',F' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF6))>0 THEN ',S' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF7))>0 THEN ',U' ELSE '' END AS \"location:deliveryDays\", EETCUSN AS customerNumber FROM PWRDTA.EETRTEAL0) AS deliveryDays ON TRIM(orderInfo.HHHCUSN) = TRIM(deliveryDays.customerNumber) LEFT JOIN PWRDTA.JJCCSTRL0 AS customerWindows ON TRIM(customerWindows.JJCCUSN) = TRIM(orderInfo.HHHCUSF) WHERE orderInfo.HHHDTEI = "+date.toString("yyyyMMdd")+" AND orderInfo.HHHCRIN<>'Y'";
    qDebug() << "route query" << queryString;
    return queryAS400(key, queryString, chunkSize);
}

bool AS400::getLocationDataForGreenmile(const QString &key, const int monthsUntilCustDisabled, const int chunkSize)
{
    QDate disableCustDate = QDate::currentDate().addMonths(-monthsUntilCustDisabled);
    qDebug() << "disable cust date" << disableCustDate << monthsUntilCustDisabled;
    QString queryString = "SELECT DISTINCT CASE WHEN customerInfo.FFDDTEI < "+disableCustDate.toString("yyyyMMdd")+" THEN 0 ELSE 1 END AS \"location:enabled\", TRIM(companyInfo.L1NAME) AS \"organization:key\", TRIM(customerInfo.FFDCUSN) AS \"location:key\", CASE WHEN TRIM(customerInfo.FFDCNMB) = '' THEN NULL ELSE TRIM(customerInfo.FFDCNMB) END AS \"location:description\", CASE WHEN TRIM(customerInfo.FFDCA1B) = '' THEN NULL ELSE TRIM(customerInfo.FFDCA1B) END AS \"location:addressLine1\", CASE WHEN TRIM(customerInfo.FFDCA2B) = '' THEN NULL ELSE TRIM(customerInfo.FFDCA2B) END AS \"location:addressLine2\", CASE WHEN TRIM(customerInfo.FFDCTYB) = '' THEN NULL ELSE TRIM(customerInfo.FFDCTYB) END AS \"location:city\", CASE WHEN TRIM(customerInfo.FFDSTEB) = '' THEN NULL ELSE TRIM(customerInfo.FFDSTEB) END AS \"location:state\", CASE WHEN TRIM(customerInfo.FFDZPCB) = '' THEN NULL ELSE TRIM(SUBSTR(customerInfo.FFDZPCB, 0, 6)) END AS \"location:zipCode\", CASE WHEN customerWindows.JJCTSR1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Open\", CASE WHEN customerWindows.JJCTSP1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Close\", CASE WHEN customerWindows.JJCTSR2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Open\", CASE WHEN customerWindows.JJCTSP2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Close\", CASE WHEN customerWindows.JJCOPEN = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCOPEN),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:openTime\", CASE WHEN customerWindows.JJCCLOS = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCCLOS),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:closeTime\", CASE WHEN TRIM(LEADING ',' FROM deliveryDays.\"location:deliveryDays\") = '' THEN NULL ELSE TRIM(LEADING ',' FROM deliveryDays.\"location:deliveryDays\") END AS \"location:deliveryDays\" FROM PWRDTA.FFDCSTBL0 AS customerInfo LEFT JOIN PWRUSER.CMPNLIST1 AS companyInfo ON companyInfo.L1LOC = TRIM(customerInfo.FFDCMPN) || TRIM(customerInfo.FFDDIVN) || TRIM (customerInfo.FFDDPTN) LEFT JOIN (SELECT CASE WHEN LENGTH(TRIM(EETRTF1))>0 THEN ',M' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF2))>0 THEN ',T' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF3))>0 THEN ',W' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF4))>0 THEN ',R' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF5))>0 THEN ',F' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF6))>0 THEN ',S' ELSE '' END || CASE WHEN LENGTH(TRIM(EETRTF7))>0 THEN ',U' ELSE '' END AS \"location:deliveryDays\", EETCUSN AS customerNumber FROM PWRDTA.EETRTEAL0) AS deliveryDays ON TRIM(customerInfo.FFDCUSN) = TRIM(deliveryDays.customerNumber) LEFT JOIN PWRDTA.JJCCSTRL0 AS customerWindows ON TRIM(customerWindows.JJCCUSN) = TRIM(customerInfo.FFDCUSN)";
    qDebug() << "cust query" << queryString;
    return queryAS400(key, queryString, chunkSize);
}

bool AS400::queryAS400(const QString &key, const QString &queryString, const int chunkSize)
{
    jsonSettings_ = settings_.loadSettings(QFile(dbPath_), jsonSettings_);
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
                processQuery(key, query, chunkSize);
            }
            else
            {
                success = false;
                emit errorMessage("AS400 Query Error: " + query.lastError().text());
                emit errorMessage("Emitting empty result set.");

                qDebug() << query.lastError().text();
                emit sqlResults(key, QMap<QString,QVariantList>());
            }

        }
        else
        {
            success = false;
            emit errorMessage("AS400 Error: " + odbc.lastError().text());
            emit errorMessage("Emitting empty result set.");

            qDebug() << "AS400 Error: " + odbc.lastError().text();
            emit sqlResults(key, QMap<QString,QVariantList>());
        }
    }
    emit debugMessage("Cleaning up AS400 connection.");
    QSqlDatabase::removeDatabase("AS400");
    return success;
}

void AS400::processQuery(const QString &key, QSqlQuery &query, const int chunkSize)
{
    bool firstRun = true;
    int recordCounter = 0;
    QMap<QString,QVariantList> sqlData;

    while(query.next())
    {
        if(recordCounter == chunkSize && chunkSize != 0)
        {
            if(sqlData.isEmpty())
            {
                emit debugMessage("AS400 query returned an empty result set.");
                qDebug() << "AS400 query returned an empty result set.";
                emit sqlResults(key, sqlData);
                qDebug() << "AS400 First run " << firstRun;
                return;
            }

            //Count the amt of records. Explodes if chunk size is zero, etc.
            emit debugMessage(QString("Retrieved " +  QString::number(sqlData.first().size()) + " records from AS400."));

            emit sqlResults(key, sqlData);
            firstRun = false;
            qDebug() << "AS400 First run " << firstRun;

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
        emit debugMessage("AS400 query returned an empty result set.");
        qDebug() << "AS400 query returned an empty result set.";
        emit sqlResults(key, sqlData);
        return;
    }

    emit debugMessage(QString("Retrieved " +  QString::number(sqlData.first().size()) + " records from AS400."));
    emit sqlResults(key, sqlData);

    for(auto key: sqlData.keys())
        sqlData[key].clear();

    sqlData.clear();
    return;
}

