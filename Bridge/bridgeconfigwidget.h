#ifndef BRIDGECONFIGWIDGET_H
#define BRIDGECONFIGWIDGET_H

#include <QWidget>
#include "Greenmile/gmconnection.h"
#include "Geocoding/censusgeocode.h"
#include "GenericUI/listcontrolwidget.h"
#include "JsonSettings/jsonsettings.h"

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
    void saveUItoSettings();
    void applySettingsToUI();

private:
    Ui::BridgeConfigWidget *ui;

    JsonSettings *jsonSettings_ = new JsonSettings(this);

    QString dbPath_ = qApp->applicationDirPath() + "/bridgeconfig.db";

    QJsonObject settings_ {{"daysToUploadInt",          QJsonValue(1)},
                           {"organization:key",         QJsonValue("SEATTLE")},
                           {"monthsUntilCustDisabled",  QJsonValue(3)},
                           {"bridgeIntervalSec",        QJsonValue(600)}};

    void init();
    void populateOrganizations(const QJsonValue &jVal);

    GMConnection *gmConn = new GMConnection(this);
};

#endif // BRIDGECONFIGWIDGET_H
