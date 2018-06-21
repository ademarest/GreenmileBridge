#ifndef BRIDGEPROGRESSWIDGET_H
#define BRIDGEPROGRESSWIDGET_H

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

private:
    Ui::BridgeProgressWidget *ui;
};

#endif // BRIDGEPROGRESSWIDGET_H
