#ifndef BRIDGECONFIGWIDGET_H
#define BRIDGECONFIGWIDGET_H

#include <QWidget>
#include "Greenmile/gmconnection.h"
#include "Geocoding/censusgeocode.h"

namespace Ui {
class BridgeConfigWidget;
}

class BridgeConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BridgeConfigWidget(QWidget *parent = Q_NULLPTR);
    ~BridgeConfigWidget();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);

private slots:
    void handleGMResponse(const QString &key, const QJsonValue &jVal);

private:
    Ui::BridgeConfigWidget *ui;
    void init();
    GMConnection *gmConn = new GMConnection(this);
    void populateOrganizations(const QJsonValue &jVal);
};

#endif // BRIDGECONFIGWIDGET_H
