#include "greenmileconfigwidget.h"
#include "ui_greenmileconfigwidget.h"

GreenmileConfigWidget::GreenmileConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GreenmileConfigWidget)
{
    ui->setupUi(this);
    connect(ui->saveSettingsButton, &QPushButton::pressed, this, &GreenmileConfigWidget::saveUItoSettings);
    applySettingsToUI();
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

void GreenmileConfigWidget::applySettingsToUI()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    ui->serverAddressLineEdit->setText(jsonSettings_["serverAddress"].toString());
    ui->usernameLineEdit->setText(jsonSettings_["username"].toString());
    ui->passwordLineEdit->setText(jsonSettings_["password"].toString());
    ui->requestTimeoutSpinbox->setValue(jsonSettings_["requestTimeoutSec"].toInt());
}

void GreenmileConfigWidget::saveUItoSettings()
{
    jsonSettings_["serverAddress"] = QJsonValue(ui->serverAddressLineEdit->text());
    jsonSettings_["username"] = QJsonValue(ui->usernameLineEdit->text());
    jsonSettings_["password"] = QJsonValue(ui->passwordLineEdit->text());
    jsonSettings_["requestTimeoutSec"] = QJsonValue(ui->requestTimeoutSpinbox->value());
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
}


