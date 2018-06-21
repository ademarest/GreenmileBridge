#include "greenmileconfigwidget.h"
#include "ui_greenmileconfigwidget.h"

GreenmileConfigWidget::GreenmileConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GreenmileConfigWidget)
{
    ui->setupUi(this);
    connect(ui->saveSettingsButton, &QPushButton::pressed, this, &GreenmileConfigWidget::saveUItoSettings);

    QJsonObject settings = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    if(noSettingsNullOrUndefined(settings))
    {
        jsonSettings_ = settings;
        applySettingsToUI(jsonSettings_);
    }
}

GreenmileConfigWidget::~GreenmileConfigWidget()
{
    delete ui;
}

bool GreenmileConfigWidget::noSettingsNullOrUndefined(const QJsonObject &settings)
{
    QSet<QString> validKeys;
    for(auto key: settings.keys())
        if(!settings[key].isNull() || !settings[key].isUndefined())
            validKeys.insert(key);

    if(validKeys.size() == settings.keys().size())
    {
        return true;
    }

    else
    {
        return false;
    }
}

void GreenmileConfigWidget::applySettingsToUI(const QJsonObject &settings)
{
    ui->serverAddressLineEdit->setText(settings["serverAddress"].toString());
    ui->usernameLineEdit->setText(settings["username"].toString());
    ui->passwordLineEdit->setText(settings["password"].toString());
}

void GreenmileConfigWidget::saveUItoSettings()
{
    jsonSettings_["serverAddress"] = QJsonValue(ui->serverAddressLineEdit->text());
    jsonSettings_["username"] = QJsonValue(ui->usernameLineEdit->text());
    jsonSettings_["password"] = QJsonValue(ui->passwordLineEdit->text());
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
    gmConn->getRouteKeysForDate(QDate::currentDate());
}
