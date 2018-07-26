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
    explicit BridgeProgressWidget(QWidget *parent = 0);
    ~BridgeProgressWidget();

public slots:
    void writeMessageTextWidget(const QString &message);
    void updateProgressBarStatus(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void addToBridgeQueue();

private:
    Ui::BridgeProgressWidget *ui;
    Bridge *bridge_ = new Bridge(this);
};

#endif // BRIDGEPROGRESSWIDGET_H
