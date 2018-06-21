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
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void saveUItoSettings();
    void handleRouteKeysForDate(QJsonArray routeArray);

private:
    Ui::GreenmileConfigWidget *ui;
    GMConnection *gmConn = new GMConnection(this);
    QString dbPath_                 = qApp->applicationDirPath() + "/gmconnection.db";
    JsonSettings *settings_         = new JsonSettings(this);
    QJsonObject jsonSettings_ {{"serverAddress", QJsonValue("https://charliesproduce.greenmile.com")},
                               {"username",      QJsonValue("username")},
                               {"password",      QJsonValue("password")}};

    bool noSettingsNullOrUndefined(const QJsonObject &settings);
    void applySettingsToUI(const QJsonObject &settings);
};

#endif // GREENMILECONFIGWIDGET_H
