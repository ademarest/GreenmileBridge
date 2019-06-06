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

AS400::~AS400()
{

}

void AS400::init()
{
    connect(&settings_, &JsonSettings::debugMessage, this, &AS400::debugMessage);
}

bool AS400::getInvoiceData(const QString &key, const QDate &minDate, const QDate &maxDate, const int chunkSize)
{
    QString queryString("SELECT "
                        "invn AS \"invoiceNumber\","
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
    QString queryString = "SELECT "
                          "oohcusn AS \"customerNumber\","
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

bool AS400::getOrganizations(const QString &key, const int chunkSize)
{
    QString queryString = "SELECT TRIM(L1NAME) AS \"organization:key\" FROM PWRUSER.CMPNLIST1";
    return queryAS400(key, queryString, chunkSize);
}

bool AS400::getRouteDataForGreenmile(const QString &key, const QDate &date, const int chunkSize)
{
    QString queryString = "SELECT DISTINCT \n"
                          "CASE WHEN customerInfo.FFDSHCD 	= '' THEN 'MISC' ELSE TRIM(customerInfo.FFDSCAC)	END AS \"locationType:key\", \n"
                          "CASE WHEN customerInfo.FFDSHCD 	= '' THEN 'MISC' ELSE TRIM(customerInfo.FFDSHCD) 	END AS \"stopType:key\", \n"
                          "CASE WHEN customerInfo.FFDSHCD 	= '' THEN 'MISC' ELSE TRIM(customerInfo.FFDSHCD) 	END AS \"accountType:key\", \n"
                          "CASE WHEN customerInfo.FFDSHCD 	= '' THEN 'MISC' ELSE TRIM(customerInfo.FFDSHCD) 	END AS \"serviceTimeType:key\", \n"
                          "TRIM(companyInfo.L1NAME) AS \"organization:key\", \n"
                          "TRIM(orderInfo.HHHRTEN) AS \"route:key\", \n"
                          "stopInfo.\"stop:baseLineSequenceNum\" AS \"stop:baseLineSequenceNum\", \n"
                          "DATE(TIMESTAMP_FORMAT(CHAR(orderInfo.HHHDTES), 'YYYY-MM-DD')) AS \"route:date\", \n"
                          "TRIM(orderInfo.HHHCUSN) || ' - ' || TRIM(orderInfo.HHHCNMB) || ' - ' || TRIM(orderInfo.HHHINVN) AS \"order:number\", \n"
                          "orderInfo.HHHQYSA AS \"order:pieces\", \n"
                          "orderInfo.HHHEXSH AS \"order:weight\", \n"
                          "orderInfo.HHHEXCB AS \"order:cube\", \n"
                          "TRIM(orderInfo.HHHCUSF) AS \"location:key\", \n"
                          "CASE WHEN TRIM(customerInfo.FFDCNMB) = '' THEN NULL ELSE TRIM(customerInfo.FFDCNMB) END AS \"location:description\", \n"
                          "CASE WHEN TRIM(customerInfo.FFDCA1B) = '' THEN NULL ELSE TRIM(customerInfo.FFDCA1B) END AS \"location:addressLine1\", \n"
                          "CASE WHEN TRIM(customerInfo.FFDCA2B) = '' THEN NULL ELSE TRIM(customerInfo.FFDCA2B) END AS \"location:addressLine2\", \n"
                          "CASE WHEN TRIM(customerInfo.FFDCTYB) = '' THEN NULL ELSE TRIM(customerInfo.FFDCTYB) END AS \"location:city\", \n"
                          "CASE WHEN TRIM(customerInfo.FFDSTEB) = '' THEN NULL ELSE TRIM(customerInfo.FFDSTEB) END AS \"location:state\", \n"
                          "CASE WHEN TRIM(customerInfo.FFDZPCB) = '' THEN NULL ELSE TRIM(customerInfo.FFDZPCB) END AS \"location:zipCode\", \n"
                          "CASE WHEN customerWindows.JJCTSR1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Open\", \n"
                          "CASE WHEN customerWindows.JJCTSP1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Close\", \n"
                          "CASE WHEN customerWindows.JJCTSR2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Open\", \n"
                          "CASE WHEN customerWindows.JJCTSP2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Close\", \n"
                          "CASE WHEN customerWindows.JJCOPEN = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCOPEN),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:openTime\", \n"
                          "CASE WHEN customerWindows.JJCCLOS = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCCLOS),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:closeTime\", \n"
                          "CASE WHEN TRIM(LEADING ',' FROM deliveryDays.\"location:deliveryDays\") = '' THEN NULL ELSE TRIM(LEADING ',' FROM deliveryDays.\"location:deliveryDays\") END AS \"location:deliveryDays\" \n"
                          "FROM PWRDTA.HHHORDHL6 AS orderInfo \n"
                          "INNER JOIN PWRUSER.CMPNLIST1 AS companyInfo \n"
                          "ON companyInfo.L1LOC = TRIM(orderInfo.HHHCMPN) || TRIM(orderInfo.HHHDIVN) || TRIM (orderInfo.HHHDPTN) \n"
                          "INNER JOIN \n"
                          "( \n"
                          "    SELECT HHHCUSF AS \"location:key\", \n"
                          "    HHHRTEN AS \"route:key\", \n"
                          "    HHHDTES AS \"route:date\", \n"
                          "    MAX(HHHSTPN) AS \"stop:baseLineSequenceNum\" \n"
                          "    FROM PWRDTA.HHHORDHL6 \n"
                          "    WHERE HHHDTES = "+date.toString("yyyyMMdd")+" \n"
                          "    GROUP BY \n"
                          "    HHHCUSF, \n"
                          "    HHHRTEN, \n"
                          "    HHHDTES \n"
                          ") AS stopInfo \n"
                          "ON TRIM(stopInfo.\"location:key\") = TRIM(orderInfo.HHHCUSF) AND TRIM(stopInfo.\"route:key\") = TRIM(orderInfo.HHHRTEN) \n"
                          "AND TRIM(stopInfo.\"route:date\") = TRIM(orderInfo.HHHDTES) \n"
                          "INNER JOIN PWRDTA.FFDCSTBL0 AS customerInfo \n"
                          "ON TRIM(customerInfo.FFDCUSN) = TRIM(orderInfo.HHHCUSF) \n"
                          "LEFT JOIN \n"
                          "( \n"
                          "    SELECT \n"
                          "    CASE WHEN LENGTH(TRIM(EETRTF1))>0 THEN ',M' ELSE '' END \n"
                          "    || CASE WHEN LENGTH(TRIM(EETRTF2))>0 THEN ',T' ELSE '' END \n"
                          "    || CASE WHEN LENGTH(TRIM(EETRTF3))>0 THEN ',W' ELSE '' END \n"
                          "    || CASE WHEN LENGTH(TRIM(EETRTF4))>0 THEN ',R' ELSE '' END \n"
                          "    || CASE WHEN LENGTH(TRIM(EETRTF5))>0 THEN ',F' ELSE '' END \n"
                          "    || CASE WHEN LENGTH(TRIM(EETRTF6))>0 THEN ',S' ELSE '' END \n"
                          "    || CASE WHEN LENGTH(TRIM(EETRTF7))>0 THEN ',U' ELSE '' END AS \"location:deliveryDays\", \n"
                          "    EETCUSN AS customerNumber \n"
                          "    FROM PWRDTA.EETRTEAL0 \n"
                          ") AS deliveryDays \n"
                          "ON TRIM(orderInfo.HHHCUSF) = TRIM(deliveryDays.customerNumber) \n"
                          "LEFT JOIN PWRDTA.JJCCSTRL0 AS customerWindows \n"
                          "ON TRIM(customerWindows.JJCCUSN) = TRIM(orderInfo.HHHCUSF) \n"
                          "WHERE orderInfo.HHHDTES = "+date.toString("yyyyMMdd")+" \n"
                          "AND orderInfo.HHHCRIN<>'Y' \n";

    qDebug() << "route query" << queryString;
    return queryAS400(key, queryString, chunkSize);
}

bool AS400::getLocationDataForGreenmile(const QString &key, const int monthsUntilCustDisabled, const int chunkSize)
{
    QDate disableCustDate = QDate::currentDate().addMonths(-monthsUntilCustDisabled);
    qDebug() << "disable cust date" << disableCustDate << monthsUntilCustDisabled;
        QString queryString = "SELECT DISTINCT \n"
                              "CASE WHEN customerInfo.FFDDTEI < "+disableCustDate.toString("yyyyMMdd")+" THEN 0 ELSE 1 END AS \"location:enabled\", \n"
                              "TRIM(companyInfo.L1NAME) AS \"organization:key\", \n"
                              "TRIM(customerInfo.FFDCUSN) AS \"location:key\", \n"
                              "CASE WHEN TRIM(customerInfo.FFDCNMB) = '' THEN NULL ELSE TRIM(customerInfo.FFDCNMB) END AS \"location:description\", \n"
                              "CASE WHEN TRIM(customerInfo.FFDCA1B) = '' THEN NULL ELSE TRIM(customerInfo.FFDCA1B) END AS \"location:addressLine1\", \n"
                              "CASE WHEN TRIM(customerInfo.FFDCA2B) = '' THEN NULL ELSE TRIM(customerInfo.FFDCA2B) END AS \"location:addressLine2\", \n"
                              "CASE WHEN TRIM(customerInfo.FFDCTYB) = '' THEN NULL ELSE TRIM(customerInfo.FFDCTYB) END AS \"location:city\", \n"
                              "CASE WHEN TRIM(customerInfo.FFDSTEB) = '' THEN NULL ELSE TRIM(customerInfo.FFDSTEB) END AS \"location:state\", \n"
                              "CASE WHEN TRIM(customerInfo.FFDZPCB) = '' THEN NULL ELSE TRIM(SUBSTR(customerInfo.FFDZPCB, 0, 6)) END AS \"location:zipCode\", \n"
                              "CASE WHEN customerWindows.JJCTSR1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Open\", \n"
                              "CASE WHEN customerWindows.JJCTSP1 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP1),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw1Close\", \n"
                              "CASE WHEN customerWindows.JJCTSR2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSR2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Open\", \n"
                              "CASE WHEN customerWindows.JJCTSP2 = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCTSP2),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:tw2Close\", \n"
                              "CASE WHEN customerWindows.JJCOPEN = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCOPEN),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:openTime\", \n"
                              "CASE WHEN customerWindows.JJCCLOS = 0 THEN NULL ELSE TIME(TIMESTAMP_FORMAT(LPAD(DIGITS(customerWindows.JJCCLOS),4,'0'),'HH24MI')) END AS \"locationOverrideTimeWindows:closeTime\", \n"
                              "CASE WHEN TRIM(deliveryDays.\"location:mondayRoute\")    = '' THEN NULL ELSE TRIM(deliveryDays.\"location:mondayRoute\")    END AS \"location:mondayRoute\", \n"
                              "CASE WHEN TRIM(deliveryDays.\"location:tuesdayRoute\")   = '' THEN NULL ELSE TRIM(deliveryDays.\"location:tuesdayRoute\")   END AS \"location:tuesdayRoute\", \n"
                              "CASE WHEN TRIM(deliveryDays.\"location:wednesdayRoute\") = '' THEN NULL ELSE TRIM(deliveryDays.\"location:wednesdayRoute\") END AS \"location:wednesdayRoute\", \n"
                              "CASE WHEN TRIM(deliveryDays.\"location:thursdayRoute\")  = '' THEN NULL ELSE TRIM(deliveryDays.\"location:thursdayRoute\")  END AS \"location:thursdayRoute\", \n"
                              "CASE WHEN TRIM(deliveryDays.\"location:fridayRoute\")    = '' THEN NULL ELSE TRIM(deliveryDays.\"location:fridayRoute\")    END AS \"location:fridayRoute\", \n"
                              "CASE WHEN TRIM(deliveryDays.\"location:saturdayRoute\")  = '' THEN NULL ELSE TRIM(deliveryDays.\"location:saturdayRoute\")  END AS \"location:saturdayRoute\", \n"
                              "CASE WHEN TRIM(deliveryDays.\"location:sundayRoute\")    = '' THEN NULL ELSE TRIM(deliveryDays.\"location:sundayRoute\")    END AS \"location:sundayRoute\", \n"
                              "CASE WHEN TRIM(LEADING ',' FROM deliveryDays.\"location:deliveryDays\") = '' THEN NULL ELSE TRIM(LEADING ',' FROM deliveryDays.\"location:deliveryDays\") END AS \"location:deliveryDays\", \n"
                              "CASE WHEN customerInfo.FFDSHCD 	= '' THEN 'MISC' ELSE TRIM(customerInfo.FFDSCAC)	END AS \"locationType:key\", \n"
                              "CASE WHEN customerInfo.FFDSHCD 	= '' THEN 'MISC' ELSE TRIM(customerInfo.FFDSHCD) 	END AS \"stopType:key\", \n"
                              "CASE WHEN customerInfo.FFDSHCD 	= '' THEN 'MISC' ELSE TRIM(customerInfo.FFDSHCD) 	END AS \"accountType:key\", \n"
                              "CASE WHEN customerInfo.FFDSHCD 	= '' THEN 'MISC' ELSE TRIM(customerInfo.FFDSHCD) 	END AS \"serviceTimeType:key\" \n"
                              "FROM PWRDTA.FFDCSTBL0 AS customerInfo \n"
                              "LEFT JOIN PWRUSER.CMPNLIST1 AS companyInfo \n"
                              "ON companyInfo.L1LOC = TRIM(customerInfo.FFDCMPN) || TRIM(customerInfo.FFDDIVN) || TRIM (customerInfo.FFDDPTN) \n"
                              "LEFT JOIN \n"
                              "( \n"
                              "    SELECT \n"
                              "    EETRTF1 AS \"location:mondayRoute\", \n"
                              "    EETRTF2 AS \"location:tuesdayRoute\", \n"
                              "    EETRTF3 AS \"location:wednesdayRoute\", \n"
                              "    EETRTF4 AS \"location:thursdayRoute\", \n"
                              "    EETRTF5 AS \"location:fridayRoute\", \n"
                              "    EETRTF6 AS \"location:saturdayRoute\", \n"
                              "    EETRTF7 AS \"location:sundayRoute\", \n"
                              "       CASE WHEN LENGTH(TRIM(EETRTF1))>0 THEN ',M' ELSE '' END \n"
                              "    || CASE WHEN LENGTH(TRIM(EETRTF2))>0 THEN ',T' ELSE '' END \n"
                              "    || CASE WHEN LENGTH(TRIM(EETRTF3))>0 THEN ',W' ELSE '' END \n"
                              "    || CASE WHEN LENGTH(TRIM(EETRTF4))>0 THEN ',R' ELSE '' END \n"
                              "    || CASE WHEN LENGTH(TRIM(EETRTF5))>0 THEN ',F' ELSE '' END \n"
                              "    || CASE WHEN LENGTH(TRIM(EETRTF6))>0 THEN ',S' ELSE '' END \n"
                              "    || CASE WHEN LENGTH(TRIM(EETRTF7))>0 THEN ',U' ELSE '' END AS \"location:deliveryDays\", \n"
                              "    EETCUSN AS customerNumber \n"
                              "    FROM PWRDTA.EETRTEAL0 \n"
                              ") AS deliveryDays \n"
                              "ON TRIM(customerInfo.FFDCUSN) = TRIM(deliveryDays.customerNumber) \n"
                              "LEFT JOIN PWRDTA.JJCCSTRL0 AS customerWindows \n"
                              "ON TRIM(customerWindows.JJCCUSN) = TRIM(customerInfo.FFDCUSN) \n";

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
                emit failed(key, query.lastError().text());
                qDebug() << query.lastError().text();
                emit sqlResults(key, QMap<QString,QVariantList>());
            }

        }
        else
        {
            success = false;
            emit errorMessage("AS400 Error: " + odbc.lastError().text());
            emit errorMessage("Emitting empty result set.");
            emit failed(key, odbc.lastError().text());
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
    int maxRecords = 0;
    QMap<QString,QVariantList> sqlData;

    while(query.next())
    {
        ++maxRecords;
    }
    query.first();
    query.previous();
    while(query.next())
    {
        if(recordCounter == chunkSize && chunkSize != 0)
        {
            if(sqlData.isEmpty())
            {
                emit debugMessage("AS400 query returned an empty result set.");
                qDebug() << "AS400 query returned an empty result set.";
                emit emptyResultSet(key);
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

    if(maxRecords != recordCounter)
    {
        emit errorMessage("Error! Lost connection midway through AS400 query. Aborting.");
        emit failed(key, "Error! Lost connection midway through AS400 query. Aborting. " + QString::number(recordCounter) + "/" + QString::number(maxRecords) + " records imported.");
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

    qDebug() << "End active?" << query.isActive();

    return;
}

