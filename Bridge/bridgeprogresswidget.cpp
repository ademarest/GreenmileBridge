#include "bridgeprogresswidget.h"
#include "ui_bridgeprogresswidget.h"

BridgeProgressWidget::BridgeProgressWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgeProgressWidget)
{
    ui->setupUi(this);
    connect(ui->bridgeButton, &QPushButton::pressed, bridge_, &Bridge::startBridge);
    connect(bridge_, &Bridge::statusMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
    connect(bridge_, &Bridge::downloadProgess, this, &BridgeProgressWidget::updateProgressBarStatus);
    connect(bridge_, &Bridge::errorMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
}

BridgeProgressWidget::~BridgeProgressWidget()
{
    delete ui;
}

void BridgeProgressWidget::writeMessageTextWidget(const QString &message)
{
    QString contents = ui->operationHistoryTextBrowser->toPlainText();
    contents.prepend(QString(QDateTime::currentDateTime().toString(Qt::ISODate) + "  " + message + "\n\n"));
    ui->operationHistoryTextBrowser->setText(contents);
}

void BridgeProgressWidget::updateProgressBarStatus(qint64 bytesReceived, qint64 bytesTotal)
{
    if(bytesTotal <= 0)
        bytesTotal = 1;

    double netStatus = (bytesReceived/bytesTotal)*100;

    if(netStatus < 0)
        netStatus = 0;

    ui->currentOperationProgressBar->setValue(netStatus);
}
