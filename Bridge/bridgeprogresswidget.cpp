#include "bridgeprogresswidget.h"
#include "ui_bridgeprogresswidget.h"

BridgeProgressWidget::BridgeProgressWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BridgeProgressWidget)
{
    ui->setupUi(this);
}

BridgeProgressWidget::~BridgeProgressWidget()
{
    delete ui;
}

void BridgeProgressWidget::writeMessageTextWidget(const QString &message)
{
    QString contents = ui->operationHistoryTextBrowser->toPlainText();
    contents.append(QString(QDateTime::currentDateTime().toString(Qt::ISODate) + "  " + message + "\n\n\n"));
    ui->operationHistoryTextBrowser->setText(contents);
}

void BridgeProgressWidget::updateProgressBarStatus(qint64 bytesReceived, qint64 bytesTotal)
{
    ui->currentOperationProgressBar->setValue(int((bytesReceived/bytesTotal)*100));
}
