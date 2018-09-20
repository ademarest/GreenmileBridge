#include "greenmileconfigwidget.h"
#include "ui_greenmileconfigwidget.h"

GreenmileConfigWidget::GreenmileConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GreenmileConfigWidget)
{
    ui->setupUi(this);
    connect(ui->saveSettingsButton, &QPushButton::pressed,  this, &GreenmileConfigWidget::saveUItoSettings);
    connect(ui->useCensusCheckBox,  &QCheckBox::clicked,    this, &GreenmileConfigWidget::switchCensusSettings);
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
    ui->maxActiveConnectionsSpinBox->setValue(jsonSettings_["maxActiveConnections"].toInt());
    ui->connectionFreqMSSpinBox->setValue(jsonSettings_["connectionFreqMS"].toInt());

    censusJsonSettings_ = censusSettings_->loadSettings(QFile(censusDBPath_), censusJsonSettings_);
    ui->censusAddressLineEdit->setText(censusJsonSettings_["serverAddress"].toString());
    ui->censusRequestTimeoutSpinBox->setValue(censusJsonSettings_["requestTimeoutSec"].toInt());
    ui->censusMaxActiveConnectionsSpinBox->setValue(censusJsonSettings_["maxActiveConnections"].toInt());
    ui->censusConnectionFreqMSSpinBox->setValue(censusJsonSettings_["connectionFreqMS"].toInt());
    ui->useCensusCheckBox->setChecked(censusJsonSettings_["useCensus"].toBool());
    qDebug() << censusJsonSettings_["useCensus"].toBool();
    qDebug() << "load" << censusJsonSettings_["useCensus"].toInt();

    switchCensusSettings(ui->useCensusCheckBox->isChecked());
}

void GreenmileConfigWidget::saveUItoSettings()
{
    jsonSettings_["serverAddress"] = QJsonValue(ui->serverAddressLineEdit->text());
    jsonSettings_["username"] = QJsonValue(ui->usernameLineEdit->text());
    jsonSettings_["password"] = QJsonValue(ui->passwordLineEdit->text());
    jsonSettings_["requestTimeoutSec"] = QJsonValue(ui->requestTimeoutSpinbox->value());
    jsonSettings_["maxActiveConnections"] = QJsonValue(ui->maxActiveConnectionsSpinBox->value());
    jsonSettings_["connectionFreqMS"] = QJsonValue(ui->connectionFreqMSSpinBox->value());
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);

    censusJsonSettings_["useCensus"] = QJsonValue(ui->useCensusCheckBox->isChecked());
    qDebug() << "save" << QJsonValue(ui->useCensusCheckBox->isChecked());
    censusJsonSettings_["serverAddress"] = QJsonValue(ui->censusAddressLineEdit->text());
    censusJsonSettings_["requestTimeoutSec"] = QJsonValue(ui->censusRequestTimeoutSpinBox->value());
    censusJsonSettings_["maxActiveConnections"] = QJsonValue(ui->censusMaxActiveConnectionsSpinBox->value());
    censusJsonSettings_["connectionFreqMS"] = QJsonValue(ui->censusConnectionFreqMSSpinBox->value());
    censusSettings_->saveSettings(QFile(censusDBPath_), censusJsonSettings_);
}

void GreenmileConfigWidget::switchCensusSettings(bool state)
{
    ui->censusAddressLabel->setEnabled(state);
    ui->censusAddressLineEdit->setEnabled(state);
    ui->censusConnectionFreqLabel->setEnabled(state);
    ui->censusConnectionFreqMSSpinBox->setEnabled(state);
    ui->censusMaxActiveConnectionsLabel->setEnabled(state);
    ui->censusMaxActiveConnectionsSpinBox->setEnabled(state);
    ui->censusRequestTimeoutLabel->setEnabled(state);
    ui->censusRequestTimeoutSpinBox->setEnabled(state);
}


