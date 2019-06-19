#include "bridgeconfigwidget.h"
#include "ui_bridgeconfigwidget.h"

BridgeConfigWidget::BridgeConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgeConfigWidget)
{
    ui->setupUi(this);
    connect(ui->saveSettingsPushButton, &QPushButton::pressed, this, &BridgeConfigWidget::saveUItoSettings);
    connect(gmConn, &GMConnection::networkResponse, this, &BridgeConfigWidget::handleGMResponse);
    connect(ui->stopTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleStopTypeIdxChange(int)));
    connect(ui->locationTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleLocationTypeIdxChange(int)));
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
    gmConn->requestAllLocationTypeInfo("allLocationTypeInfo");
    gmConn->requestAllStopTypeInfo("allStopTypeInfo");
}

void BridgeConfigWidget::handleGMResponse(const QString &key, const QJsonValue &jVal)
{
    qDebug() << key;
    if(key == "allOrganizationInfo")
    {
        populateComboBox(jVal, ui->organizationComboBox);
    }
    else if(key == "allLocationTypeInfo")
    {
        populateComboBox(jVal, ui->locationTypeComboBox, &locationTypes_);
    }
    else if(key == "allStopTypeInfo")
    {
        populateComboBox(jVal, ui->stopTypeComboBox, &stopTypes_);
    }
}

void BridgeConfigWidget::saveUItoSettings()
{
    settings_["daysToUploadInt"] = QJsonValue(ui->daysToUploadSpinBox->value());
    settings_["organization:key"] = QJsonValue(ui->organizationComboBox->currentText());
    settings_["monthsUntilCustDisabled"] = QJsonValue(ui->monthsUntilCustDisabledSpinBox->value());
    settings_["bridgeIntervalSec"] = QJsonValue(ui->bridgeIntervalSpinBox->value());\
    settings_["stopTypeDisplay"] = QJsonValue(ui->stopTypeComboBox->currentText());
    settings_["actvLocationTypeUIName"] = QJsonValue(ui->locationTypeComboBox->currentText());
    settings_["actvLocationTypeID"] = QJsonValue(ui->locationTypeIDDisplayLabel->text());

    settings_["actvStopTypeUIName"] = QJsonValue(ui->stopTypeComboBox->currentText());
    settings_["actvStopTypeID"] = QJsonValue(ui->stopTypeIDDisplayLabel->text());

    jsonSettings_->saveSettings(QFile(dbPath_), settings_);
}

void BridgeConfigWidget::applySettingsToUI()
{
    settings_ = jsonSettings_->loadSettings(QFile(dbPath_), settings_);
    ui->daysToUploadSpinBox->setValue(settings_["daysToUploadInt"].toInt());
    ui->organizationComboBox->addItem(settings_["organization:key"].toString());
    ui->monthsUntilCustDisabledSpinBox->setValue(settings_["monthsUntilCustDisabled"].toInt());
    ui->bridgeIntervalSpinBox->setValue(settings_["bridgeIntervalSec"].toInt());

    activeStopType_["uiDisplayName"] = settings_["actvStopTypeUIName"];
    activeStopType_["id"] = settings_["actvStopTypeID"];
    stopTypes_.append(activeStopType_);
    ui->stopTypeComboBox->addItem(settings_["actvStopTypeUIName"].toString());
    ui->stopTypeIDDisplayLabel->setText(settings_["actvStopTypeID"].toString());

    activeLocationType_["uiDisplayName"] = settings_["actvLocationTypeUIName"];
    activeLocationType_["id"] = settings_["actvLocationTypeID"];
    locationTypes_.append(activeLocationType_);
    ui->locationTypeComboBox->addItem(settings_["actvLocationTypeUIName"].toString());
    ui->locationTypeIDDisplayLabel->setText(settings_["actvLocationTypeID"].toString());
}

void BridgeConfigWidget::handleStopTypeIdxChange(int idx)
{
    activeStopType_ = stopTypes_[idx].toObject();
    qDebug() << stopTypes_;
    qDebug() << idx;
    qDebug() << activeStopType_;
    activeStopTypeIndex_ = idx;
    qDebug() << QString::number(activeStopType_["id"].toInt());
    ui->stopTypeIDDisplayLabel->setText(QString::number(activeStopType_["id"].toInt()));
}

void BridgeConfigWidget::handleLocationTypeIdxChange(int idx)
{
    activeLocationType_ = locationTypes_[idx].toObject();
    qDebug() << locationTypes_;
    qDebug() << locationTypes_.size();
    qDebug() << idx;
    qDebug() << activeLocationType_;
    activeLocationTypeIndex_ = idx;
    qDebug() << QString::number(activeLocationType_["id"].toInt());
    ui->locationTypeIDDisplayLabel->setText(QString::number(activeLocationType_["id"].toInt()));
}

void BridgeConfigWidget::populateComboBox(const QJsonValue &jVal, QComboBox *comboBox, QJsonArray *trackingArr)
{
    QJsonArray jArr = jVal.toArray();
    QStringList strLst;

    QStringList comboBoxContents;

    for(int i = 0; i < ui->organizationComboBox->count(); i++){
        comboBoxContents.append(comboBox->itemText(i));
    }

    for(int i = 0; i < jArr.size(); i++){

        QJsonObject jObj = jArr[i].toObject();
        QJsonObject jOrg;

        QString newContent = jObj["key"].toString();

        if(jObj.contains("organization")){
            jOrg = jObj["organization"].toObject();
            newContent.append(" - Organization: " + jOrg["key"].toString());
        }

        jObj["uiDisplayName"] = QJsonValue(newContent);


        if(!comboBoxContents.contains(newContent)){
            comboBox->addItem(newContent);

            if (trackingArr){
                trackingArr->append(jObj);
            }
        }

        jArr[i] = QJsonValue(jObj);
    }
    qDebug() << strLst;
}
