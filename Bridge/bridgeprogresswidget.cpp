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
}

BridgeProgressWidget::~BridgeProgressWidget()
{
    delete ui;
}

void BridgeProgressWidget::writeMessageTextWidget(const QString &message)
{
    QString contents = ui->operationHistoryTextBrowser->toPlainText();
    contents.append(QString(QDateTime::currentDateTime().toString(Qt::ISODate) + "  " + message + "\n\n"));
    ui->operationHistoryTextBrowser->setText(contents);
}

void BridgeProgressWidget::updateProgressBarStatus(qint64 bytesReceived, qint64 bytesTotal)
{
    ui->currentOperationProgressBar->setValue(double((bytesReceived/bytesTotal)*100));
}
