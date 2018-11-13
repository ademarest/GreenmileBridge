#ifndef AS400_H
#define AS400_H

#include "JsonSettings/jsonsettings.h"
#include <QObject>
#include <QtSql>

class AS400 : public QObject
{
    Q_OBJECT
public:
    explicit AS400(QObject *parent = nullptr);

    explicit AS400(const QString &systemIP,
                   const QString &username,
                   const QString &password,
                   QObject *parent = nullptr);

    virtual ~AS400();

    void init();

    bool getCustomerChains(const QString &key, const int chunkSize);

    bool getInvoiceData(const QString &key, const QDate &minDate,
                        const QDate &maxDate,
                        const int chunkSize);

    bool getOpenOrderHeaders(const QString &key, const int chunkSize);
    bool getOpenOrderDetails(const QString &key, const int chunkSize);
    bool getCustomerData(const QString &key);
    bool getOrganizations(const QString &key, const int chunkSize);
    bool getRouteDataForGreenmile(const QString &key, const QDate &date, const int chunkSize);
    bool getLocationDataForGreenmile(const QString &key, const int monthsUntilCustDisabled, const int chunkSize);

signals:
    void sqlResults(const QString &key, const QMap<QString,QVariantList> &sql);
    void failed(const QString &key, const QString &reason);
    void emptyResultSet(const QString &key);

    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);

private:
    bool queryAS400(const QString &key, const QString &queryString, const int chunkSize);

    void processQuery(const QString &key, QSqlQuery &query, const int chunkSize);

    void inputAS400Settings();

    QString dbPath_ = qApp->applicationDirPath() + "/as400settings.db";
    JsonSettings settings_;
    QJsonObject jsonSettings_   =   {{"username",       QJsonValue("username")},
                                     {"password",       QJsonValue("password")},
                                     {"system",         QJsonValue("0.0.0.0")},
                                     {"driver",         QJsonValue("iSeries Access ODBC Driver")}};
    //Data Formats
};

#endif // AS400_H
