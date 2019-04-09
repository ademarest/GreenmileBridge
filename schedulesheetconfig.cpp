#include "schedulesheetconfig.h"
#include "ui_schedulesheetconfig.h"

ScheduleSheetConfig::ScheduleSheetConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScheduleSheetConfig)
{
    ui->setupUi(this);
    connect(ui->addSchedPushButton,&QPushButton::pressed,this,&ScheduleSheetConfig::addNewSchedSheet);
    connect(ui->delSchedPushButton,&QPushButton::pressed,this,&ScheduleSheetConfig::deleteCurrentSchedSheet);
}

ScheduleSheetConfig::~ScheduleSheetConfig()
{
    delete ui;
}

void ScheduleSheetConfig::addNewSchedSheet()
{
    QString schedCount = QString::number(ui->schedTabWidget->count());
    QString newSchedDBFilename = "schedule"+schedCount+".db";
    QFile  newSchedDBFile(qApp->applicationDirPath() + "/" + newSchedDBFilename);

    newSchedDBFile.remove();
    MasterRouteSheetConfigWidget *mrsConfig = new MasterRouteSheetConfigWidget(newSchedDBFilename,this);
    ui->schedTabWidget->addTab(mrsConfig,"Schedule " + schedCount);
}

void ScheduleSheetConfig::deleteCurrentSchedSheet()
{

}
