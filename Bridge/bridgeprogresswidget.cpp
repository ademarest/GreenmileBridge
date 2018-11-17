#include "bridgeprogresswidget.h"
#include "ui_bridgeprogresswidget.h"

BridgeProgressWidget::BridgeProgressWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgeProgressWidget)
{
    ui->setupUi(this);
    connect(ui->bridgeButton, &QPushButton::pressed, this, &BridgeProgressWidget::addToBridgeQueue);
    connect(ui->stopBridgeButton, &QPushButton::pressed, bridgeThreadController_, &BridgeThreadController::abort);

    connect(bridgeThreadController_, &BridgeThreadController::statusMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
    connect(bridgeThreadController_, &BridgeThreadController::errorMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
    connect(bridgeThreadController_, &BridgeThreadController::currentJobChanged, ui->currentBridgeOperationLabel, &QLabel::setText);
    connect(bridgeThreadController_, &BridgeThreadController::bridgeProgress, this, &BridgeProgressWidget::updateBridgeProgressBarStatus);
    connect(bridgeThreadController_, &BridgeThreadController::currentJobProgress, this, &BridgeProgressWidget::updateBridgeJobProgressBarStatus);
    connect(bridgeThreadController_, &BridgeThreadController::bridgeKeyChanged, ui->currentBridgeKeyLabel, &QLabel::setText);
    connect(bridgeThreadController_, &BridgeThreadController::aborted, this, &BridgeProgressWidget::bridgeAborted);
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

void BridgeProgressWidget::updateBridgeProgressBarStatus(qint64 done, qint64 total)
{
    if(total <= 0)
    {
        ui->currentBridgeOperationProgressBar->setValue(0);
        return;
    }

    int netStatus = int(100) - int((double(done)/double(total))*100);

    qDebug() << netStatus << done << total;

    if(netStatus < 0)
        netStatus = 0;

    ui->currentBridgeProgressBar->setValue(netStatus);
}

void BridgeProgressWidget::updateBridgeJobProgressBarStatus(qint64 done, qint64 total)
{
    if(total <= 0)
    {
        ui->currentBridgeOperationProgressBar->setValue(0);
        return;
    }

    int netStatus = int(100) - int((double(done)/double(total))*100);

    qDebug() << netStatus << done << total;

    if(netStatus < 0)
        netStatus = 0;

    ui->currentBridgeOperationProgressBar->setValue(netStatus);
}

void BridgeProgressWidget::addToBridgeQueue()
{
    bridgeThreadController_->addRequest("USER_INITIATED_BRIDGE");
}

void BridgeProgressWidget::bridgeAborted()
{
    ui->currentBridgeOperationProgressBar->setValue(0);
    ui->currentBridgeProgressBar->setValue(0);
}
