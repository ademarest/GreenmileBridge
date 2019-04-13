#include "schedconfirmdeletedialog.h"
#include "ui_schedconfirmdeletedialog.h"

SchedConfirmDeleteDialog::SchedConfirmDeleteDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SchedConfirmDeleteDialog)
{
    ui->setupUi(this);
}

SchedConfirmDeleteDialog::~SchedConfirmDeleteDialog()
{
    delete ui;
}
