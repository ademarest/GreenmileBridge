#include "as400configwidget.h"
#include "ui_as400configwidget.h"

AS400ConfigWidget::AS400ConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AS400ConfigWidget)
{
    ui->setupUi(this);
    connect(ui->commitSettings, &QPushButton::pressed, this, &AS400ConfigWidget::saveUItoSettings);
    applySettingsToUI();
}

AS400ConfigWidget::~AS400ConfigWidget()
{
    delete ui;
}

void AS400ConfigWidget::saveUItoSettings()
{
    jsonSettings_["password"] = ui->passwordLineEdit->text();
    jsonSettings_["username"] = ui->usernameLineEdit->text();
    jsonSettings_["driver"] = ui->driverLineEdit->text().simplified();
    jsonSettings_["system"] = ui->systemLineEdit->text().simplified();
    settings_.saveSettings(QFile(dbPath_), jsonSettings_);
}

bool AS400ConfigWidget::noSettingsNullOrUndefined(const QJsonObject &settings)
{
    QSet<QString> validKeys;
    for(auto key: settings.keys())
        if(!settings[key].isNull() || !settings[key].isUndefined())
            validKeys.insert(key);

    if(validKeys.size() == settings.keys().size())
        return true;

    else
        return false;
}

void AS400ConfigWidget::applySettingsToUI()
{
    jsonSettings_ = settings_.loadSettings(QFile(dbPath_), jsonSettings_);
    ui->passwordLineEdit->setText(jsonSettings_["password"].toString());
    ui->usernameLineEdit->setText(jsonSettings_["username"].toString());
    ui->driverLineEdit->setText(jsonSettings_["driver"].toString());
    ui->systemLineEdit->setText(jsonSettings_["system"].toString());
}
