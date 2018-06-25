#ifndef AS400CONFIGWIDGET_H
#define AS400CONFIGWIDGET_H

#include "JsonSettings/jsonsettings.h"
#include <QWidget>

namespace Ui {
class AS400ConfigWidget;
}

class AS400ConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AS400ConfigWidget(QWidget *parent = 0);
    ~AS400ConfigWidget();

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);

private slots:
    void saveUItoSettings();

private:
    Ui::AS400ConfigWidget *ui;
    QString dbPath_ = qApp->applicationDirPath() + "/as400settings.db";
    JsonSettings settings_;
    QJsonObject jsonSettings_   =   {{"username",       QJsonValue("username")},
                                     {"password",       QJsonValue("password")},
                                     {"system",         QJsonValue("0.0.0.0")},
                                     {"driver",         QJsonValue("iSeries Access ODBC Driver")}};

    bool noSettingsNullOrUndefined(const QJsonObject &settings);
    void applySettingsToUI();
};

#endif // AS400CONFIGWIDGET_H
