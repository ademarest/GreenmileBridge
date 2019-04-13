#include "addscheduledialog.h"
#include "ui_addscheduledialog.h"

AddScheduleDialog::AddScheduleDialog(int tabIndex,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddScheduleDialog)
{
    QString schedCount = QString::number(tabIndex);

    ui->setupUi(this);
    ui->scheduleNameLineEdit->setText("New Schedule " + schedCount);
    connect(ui->buttonBox,&QDialogButtonBox::accepted,this,&AddScheduleDialog::setScheduleName);
}

AddScheduleDialog::~AddScheduleDialog()
{
    delete ui;
}

void AddScheduleDialog::setScheduleName()
{
    emit scheduleName(ui->scheduleNameLineEdit->text());
}
