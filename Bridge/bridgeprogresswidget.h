#ifndef BRIDGEPROGRESSWIDGET_H
#define BRIDGEPROGRESSWIDGET_H

#include <QObject>
#include <QtCore>
#include <QWidget>

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

private:
    Ui::BridgeProgressWidget *ui;
};

#endif // BRIDGEPROGRESSWIDGET_H
