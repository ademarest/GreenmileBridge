#ifndef BRIDGEPROGRESSWIDGET_H
#define BRIDGEPROGRESSWIDGET_H

#include <QtCore>
#include <QWidget>
#include "bridge.h"

namespace Ui {
class BridgeProgressWidget;
}

class BridgeProgressWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BridgeProgressWidget(QWidget *parent = Q_NULLPTR);
    ~BridgeProgressWidget();

public slots:
    void writeMessageTextWidget(const QString &message);
    void updateBridgeProgressBarStatus(qint64 done, qint64 total);
    void updateBridgeJobProgressBarStatus(qint64 done, qint64 total);

private slots:
    void addToBridgeQueue();

private:
    Ui::BridgeProgressWidget *ui;
    Bridge *bridge_ = new Bridge(this);
};

#endif // BRIDGEPROGRESSWIDGET_H
