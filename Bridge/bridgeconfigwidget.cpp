#include "bridgeconfigwidget.h"
#include "ui_bridgeconfigwidget.h"

BridgeConfigWidget::BridgeConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgeConfigWidget)
{
    ui->setupUi(this);
    connect(ui->saveSettingsPushButton, &QPushButton::pressed, this, &BridgeConfigWidget::saveUItoSettings);
    connect(gmConn, &GMConnection::networkResponse, this, &BridgeConfigWidget::handleGMResponse);
    init();
}


BridgeConfigWidget::~BridgeConfigWidget()
{
    delete ui;
}

void BridgeConfigWidget::init()
{
    applySettingsToUI();
    gmConn->requestAllOrganizationInfo("allOrganizationInfo");
}

void BridgeConfigWidget::handleGMResponse(const QString &key, const QJsonValue &jVal)
{
    qDebug() << key;
    if(key == "allOrganizationInfo")
    {
        populateOrganizations(jVal);
    }
}

void BridgeConfigWidget::saveUItoSettings()
{
    settings_["daysToUploadInt"] = QJsonValue(ui->daysToUploadSpinBox->value());
    settings_["organization:key"] = QJsonValue(ui->organizationComboBox->currentText());
    settings_["monthsUntilCustDisabled"] = QJsonValue(ui->monthsUntilCustDisabledSpinBox->value());
    settings_["bridgeIntervalSec"] = QJsonValue(ui->bridgeIntervalSpinBox->value());
    jsonSettings_->saveSettings(QFile(dbPath_), settings_);
}

void BridgeConfigWidget::applySettingsToUI()
{
    settings_ = jsonSettings_->loadSettings(QFile(dbPath_), settings_);
    ui->daysToUploadSpinBox->setValue(settings_["daysToUploadInt"].toInt());
    ui->organizationComboBox->addItem(settings_["organization:key"].toString());
    ui->monthsUntilCustDisabledSpinBox->setValue(settings_["monthsUntilCustDisabled"].toInt());
    ui->bridgeIntervalSpinBox->setValue(settings_["bridgeIntervalSec"].toInt());
}

void BridgeConfigWidget::populateOrganizations(const QJsonValue &jVal)
{
    QJsonArray jArr = jVal.toArray();
    QStringList strLst;

    QStringList comboBoxContents;

    for(int i = 0; i < ui->organizationComboBox->count(); i++){
        comboBoxContents.append(ui->organizationComboBox->itemText(i));
    }

    for(auto jValTemp:jArr)
    {
        QJsonObject jObj = jValTemp.toObject();

        QString newContent = jObj["key"].toString();

        if(!comboBoxContents.contains(newContent)){
            ui->organizationComboBox->addItem(newContent);
        }
    }

    qDebug() << strLst;

}
