#include "bridgeconfigwidget.h"
#include "ui_bridgeconfigwidget.h"

BridgeConfigWidget::BridgeConfigWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgeConfigWidget)
{
    ui->setupUi(this);
    connect(gmConn, &GMConnection::gmNetworkResponse, this, &BridgeConfigWidget::handleGMResponse);
    connect(as400Conn, &AS400::sqlResults, this, &BridgeConfigWidget::handleSQLResponse);
    init();
}


BridgeConfigWidget::~BridgeConfigWidget()
{
    delete ui;
}

void BridgeConfigWidget::init()
{
    gmConn->requestAllOrganizationInfo("allOrganizationInfo");
    as400Conn->getOrganizations("as400Organizations", 1000);
}

void BridgeConfigWidget::handleGMResponse(const QString &key, const QJsonValue &jVal)
{
    qDebug() << key;
    if(key == "allOrganizationInfo")
    {
        populateOrganizations(jVal);
    }

}

void BridgeConfigWidget::handleSQLResponse(const QString &key, const QMap<QString, QVariantList> &sql)
{
    qDebug() << key;
    QStringList strList;
    for(int i = 0; i < sql["organization:key"].size(); ++i)
    {
        ui->orgAS400LCW->appendItem(sql["organization:key"][i].toString());
        //as400OrgLCW->appendItem("BridgeConfig add item", (QString(QString::number(i+1) + " " + sql["organization:key"][i].toString())));
    }
    qDebug() << strList;

}


void BridgeConfigWidget::populateOrganizations(const QJsonValue &jVal)
{
    QJsonArray jArr = jVal.toArray();
    QStringList strLst;
    for(auto jValTemp:jArr)
    {
        QJsonObject jObj = jValTemp.toObject();
        ui->organizationComboBox->addItem(jObj["key"].toString());
        //ui->orgGMLCW->appendItem("populate gm org", jObj["key"].toString());
        //gmOrgLCW->appendItem("BridgeConfig add item", jObj["key"].toString());
    }
    qDebug() << strLst;

}
