#ifndef SCHEDULESHEETCONFIG_H
#define SCHEDULESHEETCONFIG_H

#include "Bridge/MasterRoute/masterroutesheetconfigwidget.h"
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

public slots:
    void addNewSchedSheet();
    void deleteCurrentSchedSheet();

private:
    Ui::ScheduleSheetConfig *ui;

    QJsonObject settings_ {{"scheduleTables", QJsonValue(QJsonArray{QJsonValue(QJsonObject{{"tableName", QJsonValue("dlmrsDailyAssignments")}}),
                                                                    QJsonValue(QJsonObject{{"tableName", QJsonValue("mrsDailyAssignments")}, {"minRouteKey", "D"}, {"maxRouteKey", "U"}})})}};

};

#endif // SCHEDULESHEETCONFIG_H
