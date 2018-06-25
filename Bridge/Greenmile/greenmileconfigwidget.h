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
    explicit GreenmileConfigWidget(QWidget *parent = 0);
    ~GreenmileConfigWidget();

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);

private slots:
    void saveUItoSettings();

private:
    Ui::GreenmileConfigWidget *ui;
    QString dbPath_                 = qApp->applicationDirPath() + "/gmconnection.db";
    JsonSettings *settings_         = new JsonSettings(this);
    QJsonObject jsonSettings_ {{"serverAddress",        QJsonValue("https://charliesproduce.greenmile.com")},
                               {"username",             QJsonValue("username")},
                               {"password",             QJsonValue("password")},
                               {"requestTimeoutSec",    QJsonValue(40)}};

    bool noSettingsNullOrUndefined(const QJsonObject &settings);
    void applySettingsToUI();
};

#endif // GREENMILECONFIGWIDGET_H
