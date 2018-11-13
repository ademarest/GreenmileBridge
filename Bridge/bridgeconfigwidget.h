#ifndef BRIDGECONFIGWIDGET_H
#define BRIDGECONFIGWIDGET_H

#include <QWidget>
#include "Greenmile/gmconnection.h"
#include "AS400/as400connection.h"
#include "Geocoding/censusgeocode.h"
#include "GenericUI/listcontrolwidget.h"

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
    void handleSQLResponse(const QString &key, const QMap<QString, QVariantList> &sql);

private:
    Ui::BridgeConfigWidget *ui;
    void init();
    void populateOrganizations(const QJsonValue &jVal);

    //ListControlWidget *gmOrgLCW     = new ListControlWidget(this);
    //ListControlWidget *as400OrgLCW  = new ListControlWidget(this);

    AS400 *as400Conn = new AS400(this);
    GMConnection *gmConn = new GMConnection(this);
};

#endif // BRIDGECONFIGWIDGET_H
