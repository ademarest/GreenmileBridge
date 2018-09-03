#ifndef BRIDGETHREADCONTROLLER_H
#define BRIDGETHREADCONTROLLER_H

#include "bridge.h"

class BridgeThreadController : public QObject
{
    Q_OBJECT
    QThread bridgeThread_;
public:
    explicit BridgeThreadController(QObject *parent = nullptr);
    ~BridgeThreadController();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);

    void started(const QString &key);
    void aborted(const QString &key);
    void rebuilt(const QString &key);
    void finished(const QString &key);

    void bridgeKeyChanged(const QString &key);
    void bridgeProgress(const int remainingWork, const int totalWork);
    void currentJobProgress(const int remainingWork, const int totalWork);
    void currentJobChanged(const QString &key);

    void addRequest(const QString &key);
    void removeRequest(const QString &key);
    void abort();
    bool hasActiveJobs();

public slots:

};

#endif // BRIDGETHREADCONTROLLER_H

//connect(ui->bridgeButton, &QPushButton::pressed, this, &BridgeProgressWidget::addToBridgeQueue);
//connect(ui->stopBridgeButton, &QPushButton::pressed, bridge_, &Bridge::abort);

//connect(bridge, &Bridge::statusMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
//connect(bridge, &Bridge::errorMessage, this, &BridgeProgressWidget::writeMessageTextWidget);
//connect(bridge, &Bridge::currentJobChanged, ui->currentBridgeOperationLabel, &QLabel::setText);
//connect(bridge, &Bridge::bridgeProgress, this, &BridgeProgressWidget::updateBridgeProgressBarStatus);
//connect(bridge, &Bridge::currentJobProgress, this, &BridgeProgressWidget::updateBridgeJobProgressBarStatus);
//connect(bridge, &Bridge::bridgeKeyChanged, ui->currentBridgeKeyLabel, &QLabel::setText);
