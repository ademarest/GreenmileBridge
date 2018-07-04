#ifndef MASTERROUTESHEETDATACONFIGWIDGET_H
#define MASTERROUTESHEETDATACONFIGWIDGET_H

#include "JsonSettings/jsonsettings.h"
#include <QWidget>
#include <QFileDialog>
#include <QInputDialog>

namespace Ui {
class MasterRouteSheetDataConfigWidget;
}

class MasterRouteSheetDataConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MasterRouteSheetDataConfigWidget(QWidget *parent = 0);
    ~MasterRouteSheetDataConfigWidget();

private slots:
    void saveUItoSettings();
    void loadSettingsFromFile();
    void addRedirectURI();
    void removeRedirectURI();

private:
    Ui::MasterRouteSheetDataConfigWidget *ui;

    JsonSettings *settings_ = new JsonSettings(this);
    QString dbPath_ = qApp->applicationDirPath() + "/mrsdataconnection.db";
    QJsonObject jsonSettings_  {{"client_id", QJsonValue("Client ID")},
                                {"auth_uri", QJsonValue("Authentication URI")},
                                {"token_uri", QJsonValue("Token URI")},
                                {"auth_provider_x509_cert_url", QJsonValue("x509 Certificate URL")},
                                {"project_id", QJsonValue("Project ID")},
                                {"client_secret", QJsonValue("Client Secret")},
                                {"api_scope", QJsonValue("https://www.googleapis.com/auth/spreadsheets.readonly")},
                                {"base_url", QJsonValue("e.g. https://sheets.googleapis.com/v4/spreadsheets/<someString>/values/")},
                                {"redirect_uris", QJsonArray()},
                                {"request_timeout", QJsonValue(40)},
                                {"oauth2_user_timeout", QJsonValue(180)}};

    bool noSettingsNullOrUndefined(const QJsonObject &settings);
    void applySettingsToUI();
    QJsonObject makeJsonFromFile(const QString &jsonCredentialPath);
};

#endif // MASTERROUTESHEETDATACONFIGWIDGET_H
