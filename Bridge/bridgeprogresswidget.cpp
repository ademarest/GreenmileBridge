#include "bridgeprogresswidget.h"
#include "ui_bridgeprogresswidget.h"

BridgeProgressWidget::BridgeProgressWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgeProgressWidget)
{
    ui->setupUi(this);
    connect(ui->bridgeButton, &QPushButton::pressed, this, &BridgeProgressWidget::addToBridgeQueue);
    connect(ui->stopBridgeButton, &QPushButton::pressed, bridge_, &Bridge::abort);

    connect(bridge_, &Bridge::statusMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
    connect(bridge_, &Bridge::errorMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
    connect(bridge_, &Bridge::currentJobChanged, ui->currentBridgeOperationLabel, &QLabel::setText);
    connect(bridge_, &Bridge::bridgeProgress, this, &BridgeProgressWidget::updateBridgeProgressBarStatus);
    connect(bridge_, &Bridge::currentJobProgress, this, &BridgeProgressWidget::updateBridgeJobProgressBarStatus);
    connect(bridge_, &Bridge::bridgeKeyChanged, ui->currentBridgeKeyLabel, &QLabel::setText);
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
    bridge_->addRequest("USER_INITIATED_BRIDGE");
}
