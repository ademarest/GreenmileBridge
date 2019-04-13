#include "schedulesheetconfig.h"
#include "ui_schedulesheetconfig.h"

ScheduleSheetConfig::ScheduleSheetConfig(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScheduleSheetConfig)
{
    ui->setupUi(this);
    applySettingsToUI();
    connect(ui->addSchedPushButton,&QPushButton::pressed,this,&ScheduleSheetConfig::addNewSchedSheet);
    connect(ui->delSchedPushButton,&QPushButton::pressed,this,&ScheduleSheetConfig::deleteCurrentSchedSheet);
}

ScheduleSheetConfig::~ScheduleSheetConfig()
{
    for(auto sched:schedulesToDelete_){
        deleteDatabase(sched);
    }

    settings_->saveSettings(QFile(dbPath_),jsonSettings_);

    delete ui;
}

void ScheduleSheetConfig::addNewSchedSheet()
{
    AddScheduleDialog *asd = createScheduleSheetDialog();
    connect(asd,&AddScheduleDialog::scheduleName,this,&ScheduleSheetConfig::createNewScheduleSheet);
}

AddScheduleDialog * ScheduleSheetConfig::createScheduleSheetDialog()
{
    AddScheduleDialog *asd = new AddScheduleDialog(ui->schedTabWidget->count(), this);
    asd->setAttribute(Qt::WA_DeleteOnClose,true);
    asd->show();
    return asd;
}

void ScheduleSheetConfig::deleteDatabase(const QString &name)
{
    qDebug() << "deleting " << name;
    QString newSchedDBFilename = name+".db";
    QFile   newSchedDBFile(qApp->applicationDirPath() + "/" + newSchedDBFilename);
    newSchedDBFile.remove();
}

void ScheduleSheetConfig::createNewScheduleSheet(const QString &name)
{
    if(schedulesToDelete_.contains(name)){
        schedulesToDelete_.removeAt(schedulesToDelete_.indexOf(name));
    }

    QString newSchedDBFilename = name+".db";
    //deleteDatabase(name);
    MasterRouteSheetConfigWidget *mrsConfig = new MasterRouteSheetConfigWidget(newSchedDBFilename, this);
    ui->schedTabWidget->addTab(mrsConfig,name);
    saveUItoSettings();
}

void ScheduleSheetConfig::loadScheduleSheet(const QString name)
{
    QString newSchedDBFilename = name+".db";
    MasterRouteSheetConfigWidget *mrsConfig = new MasterRouteSheetConfigWidget(newSchedDBFilename, this);
    ui->schedTabWidget->addTab(mrsConfig,name);
}

void ScheduleSheetConfig::deleteCurrentSchedSheet()
{
    QJsonArray scheduleList = jsonSettings_["scheduleList"].toArray();

    int tabIndex = ui->schedTabWidget->currentIndex();
    QString currentTabTitle = ui->schedTabWidget->tabText(tabIndex);
    ui->schedTabWidget->removeTab(tabIndex);

    for(int i = 0; i < scheduleList.size(); i++){
        if(scheduleList[i].toObject()["tableName"].toString() == currentTabTitle){
            scheduleList.removeAt(i);
        }
    }

    qDebug() << scheduleList;
    jsonSettings_["scheduleList"] = scheduleList;

    schedulesToDelete_ << currentTabTitle;
}

void ScheduleSheetConfig::applySettingsToUI()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_),jsonSettings_);
    qDebug() << "loading settings" <<  jsonSettings_;

    for(auto schedule:jsonSettings_["scheduleList"].toArray()){
        loadScheduleSheet(schedule.toObject()["tableName"].toString());
    }
}

void ScheduleSheetConfig::saveUItoSettings()
{
    qDebug() << "saving sheets";
    QJsonArray scheduleList;
    int tabCount = ui->schedTabWidget->count();

    for(int i = 0; i < tabCount; i++){
        QJsonObject scheduleObj;
        scheduleObj["tableName"] = ui->schedTabWidget->tabText(i);
        scheduleList.append(QJsonValue(scheduleObj));
    }

    jsonSettings_["scheduleList"] = scheduleList;

    qDebug() << jsonSettings_;
    settings_->saveSettings(QFile(dbPath_),jsonSettings_);
}
