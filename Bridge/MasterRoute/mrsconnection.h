#ifndef MRSCONNECTION_H
#define MRSCONNECTION_H

#include "JsonSettings/jsonsettings.h"
#include "Bridge/GoogleSheets/googlesheetsconnection.h"

class MRSConnection : public QObject
{
    Q_OBJECT
public:
    explicit MRSConnection(const QString &databaseName, QObject *parent = nullptr);
    virtual ~MRSConnection();
    void requestAssignments(const QString &key, const QDate &date);
    void requestAssignments(const QString &key, const QString &sheetName);

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void mrsDailyScheduleSQL(const QString &key,const QMap<QString,QVariantList> &sql);
    void failed(const QString &key, const QString &reason);

private slots:
    void handleNetworkReply(const QString &key, const QJsonObject &data);

private:
    QMap<QString,QVariantList> mrsDailyScheduleJsonToSQL(const QJsonObject &data);
    GoogleSheetsConnection *googleSheets_;
    JsonSettings *settings_ = new JsonSettings(this);
    QString dbPath_;
    QJsonObject jsonSettings_  {{"organization_key", QJsonValue("SEATTLE")},
                                {"date_format", QJsonValue("d-MMM-yyyy")},
                                {"driver_offset", QJsonValue(1)},
                                {"truck_offset", QJsonValue(2)},
                                {"trailer_offset", QJsonValue(3)},
                                {"client_id", QJsonValue()},
                                {"auth_uri", QJsonValue()},
                                {"token_uri", QJsonValue()},
                                {"auth_provider_x509_cert_url", QJsonValue()},
                                {"project_id", QJsonValue()},
                                {"client_secret", QJsonValue()},
                                {"api_scope", QJsonValue()},
                                {"base_url", QJsonValue()},
                                {"redirect_uris", QJsonArray()},
                                {"token", QJsonValue()},
                                {"expiration_at", QJsonValue()},
                                {"refresh_token", QJsonValue()},
                                {"request_timeout", QJsonValue(40)},
                                {"oauth2_user_timeout", QJsonValue(180)}};
};

#endif // MRSCONNECTION_H
