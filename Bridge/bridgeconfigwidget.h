#ifndef BRIDGECONFIGWIDGET_H
#define BRIDGECONFIGWIDGET_H

#include <QWidget>
#include "Greenmile/gmconnection.h"

namespace Ui {
class BridgeConfigWidget;
}

class BridgeConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BridgeConfigWidget(QWidget *parent = 0);
    ~BridgeConfigWidget();

private slots:
    void handleGMResponse(const QString &key, const QJsonValue &jVal);

private:
    Ui::BridgeConfigWidget *ui;
    GMConnection *gmConn = new GMConnection(this);
    void populateOrganizations(const QJsonValue &jVal);
};

#endif // BRIDGECONFIGWIDGET_H
