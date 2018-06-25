#ifndef AS400INPUTWIDGET_H
#define AS400INPUTWIDGET_H

#include "JsonSettings/jsonsettings.h"
#include <QWidget>

namespace Ui {
class AS400InputWidget;
}

class AS400InputWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AS400InputWidget(QWidget *parent = 0);
    ~AS400InputWidget();

private slots:
    void saveSettings();
    void loadSettings();

private:
    Ui::AS400InputWidget *ui;
    QString dbPath_ = qApp->applicationDirPath() + "/as400settings.db";
    JsonSettings settings_;
    QJsonObject AS400Settings_ =    {{"username",       QJsonValue("username")},
                                     {"password",       QJsonValue("password")},
                                     {"system",         QJsonValue("0.0.0.0")},
                                     {"driver",         QJsonValue("iSeries Access ODBC Driver")}};
};

#endif // AS400INPUTWIDGET_H
