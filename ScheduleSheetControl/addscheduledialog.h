#ifndef ADDSCHEDULEDIALOG_H
#define ADDSCHEDULEDIALOG_H

#include <QDialog>

namespace Ui {
class AddScheduleDialog;
}

class AddScheduleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddScheduleDialog(int tabIndex, QWidget *parent = nullptr);
    ~AddScheduleDialog();

private slots:
    void setScheduleName();

signals:
    void scheduleName(const QString &scheduleName);

private:
    Ui::AddScheduleDialog *ui;
};

#endif // ADDSCHEDULEDIALOG_H
