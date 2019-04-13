#ifndef SCHEDCONFIRMDELETEDIALOG_H
#define SCHEDCONFIRMDELETEDIALOG_H

#include <QDialog>

namespace Ui {
class SchedConfirmDeleteDialog;
}

class SchedConfirmDeleteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SchedConfirmDeleteDialog(QWidget *parent = nullptr);
    ~SchedConfirmDeleteDialog();

private:
    Ui::SchedConfirmDeleteDialog *ui;
};

#endif // SCHEDCONFIRMDELETEDIALOG_H
