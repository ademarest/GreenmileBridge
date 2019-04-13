#ifndef SCHEDULESHEETCONFIG_H
#define SCHEDULESHEETCONFIG_H

#include "Bridge/MasterRoute/masterroutesheetconfigwidget.h"
#include "ScheduleSheetControl/addscheduledialog.h"
#include "JsonSettings/jsonsettings.h"
#include <QWidget>

namespace Ui {
class ScheduleSheetConfig;
}

class ScheduleSheetConfig : public QWidget
{
    Q_OBJECT

public:
    explicit ScheduleSheetConfig(QWidget *parent = nullptr);
    ~ScheduleSheetConfig();

private slots:
    void addNewSchedSheet();
    void deleteCurrentSchedSheet();
    void saveUItoSettings();

private:
    Ui::ScheduleSheetConfig *ui;

    JsonSettings *settings_ = new JsonSettings(this);
    QString dbPath_ = qApp->applicationDirPath() + "/scheduleconfig.db";
    QJsonObject jsonSettings_ {{"scheduleList",QJsonArray()}};
    QStringList schedulesToDelete_;

    AddScheduleDialog *createScheduleSheetDialog();
    void deleteDatabase(const QString &name);
    void createNewScheduleSheet(const QString &name);
    void applySettingsToUI();

    void loadScheduleSheet(const QString name);
};

#endif // SCHEDULESHEETCONFIG_H
