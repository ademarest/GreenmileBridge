#ifndef GREENMILECONFIGWIDGET_H
#define GREENMILECONFIGWIDGET_H

#include <QWidget>
#include "JsonSettings/jsonsettings.h"
#include "gmconnection.h"

namespace Ui {
class GreenmileConfigWidget;
}

class GreenmileConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GreenmileConfigWidget(QWidget *parent = Q_NULLPTR);
    ~GreenmileConfigWidget();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);

private slots:
    void saveUItoSettings();
    void switchCensusSettings(bool state);

private:
    Ui::GreenmileConfigWidget *ui;
    QString dbPath_                 = qApp->applicationDirPath() + "/gmconnection.db";
    JsonSettings *settings_         = new JsonSettings(this);
    QJsonObject jsonSettings_ {{"serverAddress",        QJsonValue("https://charliesproduce.greenmile.com")},
                               {"username",             QJsonValue("username")},
                               {"password",             QJsonValue("password")},
                               {"requestTimeoutSec",    QJsonValue(40)},
                               {"maxActiveConnections", QJsonValue(10)},
                               {"connectionFreqMS",     QJsonValue(100)}};

    QString censusDBPath_                 = qApp->applicationDirPath() + "/census.db";
    JsonSettings *censusSettings_         = new JsonSettings(this);
    QJsonObject censusJsonSettings_     {{"useCensus",           QJsonValue(false)},
                                        {"serverAddress",        QJsonValue("https://geocoding.geo.census.gov")},
                                        {"requestTimeoutSec",    QJsonValue(40)},
                                        {"maxActiveConnections", QJsonValue(10)},
                                        {"connectionFreqMS",     QJsonValue(100)}};

    bool noSettingsNullOrUndefined(const QJsonObject &settings);
    void applySettingsToUI();
};

#endif // GREENMILECONFIGWIDGET_H
