#include "bridgeprogresswidget.h"
#include "ui_bridgeprogresswidget.h"

BridgeProgressWidget::BridgeProgressWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgeProgressWidget)
{
    ui->setupUi(this);
    connect(ui->bridgeButton, &QPushButton::pressed, this, &BridgeProgressWidget::addToBridgeQueue);
    connect(bridge_, &Bridge::statusMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
    connect(bridge_, &Bridge::errorMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
}

BridgeProgressWidget::~BridgeProgressWidget()
{
    delete ui;
}

void BridgeProgressWidget::writeMessageTextWidget(const QString &message)
{
    QString contents = ui->operationHistoryTextBrowser->toPlainText();

    //Prevent the widget from keeling over and dying.
    if(contents.size() > 200000)
        contents.remove(100000,(contents.size() - 1));

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

void BridgeProgressWidget::addToBridgeQueue()
{
    bridge_->addRequest("USER_INITIATED_BRIDGE");
}
