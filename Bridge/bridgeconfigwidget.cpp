#include "bridgeconfigwidget.h"
#include "ui_bridgeconfigwidget.h"

BridgeConfigWidget::BridgeConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgeConfigWidget)
{
    ui->setupUi(this);
    connect(gmConn, &GMConnection::gmNetworkResponse, this, &BridgeConfigWidget::handleGMResponse);
    gmConn->requestAllOrganizationInfo();
}

BridgeConfigWidget::~BridgeConfigWidget()
{
    delete ui;
}

void BridgeConfigWidget::handleGMResponse(const QString &key, const QJsonValue &jVal)
{
    if(key == "allOrganizationInfo")
    {
        populateOrganizations(jVal);
    }

}

void BridgeConfigWidget::populateOrganizations(const QJsonValue &jVal)
{
    QJsonArray jArr = jVal.toArray();
    for(auto jValTemp:jArr)
    {
        QJsonObject jObj = jValTemp.toObject();
        ui->organizationComboBox->addItem(jObj["key"].toString());
    }
}
