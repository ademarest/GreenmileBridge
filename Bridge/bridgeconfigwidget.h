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
    void handleStopTypeIdxChange(int idx);
    void handleLocationTypeIdxChange(int idx);

private:
    Ui::BridgeConfigWidget *ui;

    JsonSettings *jsonSettings_ = new JsonSettings(this);

    QString dbPath_ = qApp->applicationDirPath() + "/bridgeconfig.db";

    QJsonObject entityType_ = {
        {"id", "00000"},
        {"uiDisplayName","Default, please change this."}
    };

    QJsonArray entityArray_ = {entityType_};

    QJsonObject settings_ {{"daysToUploadInt",           QJsonValue(1)},
                           {"organization:key",          QJsonValue("Default organization, please change this.")},
                           {"monthsUntilCustDisabled",   QJsonValue(3)},
                           {"bridgeIntervalSec",         QJsonValue(600)},
                           {"actvLocationTypeID",        QJsonValue("00000")},
                           {"actvLocationTypeUIName",    QJsonValue("Default location type, please change this.")},
                           {"actvStopTypeID",            QJsonValue("00000")},
                           {"actvStopTypeUIName",        QJsonValue("Default stop type, please change this.")}};

    int activeLocationTypeIndex_;
    QJsonObject activeLocationType_;
    QJsonArray locationTypes_;

    int activeStopTypeIndex_;
    QJsonObject activeStopType_;
    QJsonArray stopTypes_;

    void init();
    void populateComboBox(const QJsonValue &jVal, QComboBox *comboBox, QJsonArray *trackingArr = Q_NULLPTR);

    GMConnection *gmConn = new GMConnection(this);
};

#endif // BRIDGECONFIGWIDGET_H
