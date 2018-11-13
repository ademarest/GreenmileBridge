#ifndef LISTCONTROLWIDGET_H
#define LISTCONTROLWIDGET_H

#include <QtCore>
#include <QWidget>

namespace Ui {
class ListControlWidget;
}

class ListControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ListControlWidget(QWidget *parent = nullptr);
    ~ListControlWidget();

public slots:
    void addItems(const QString &key, const QStringList &items);
    void appendItem(const QString &key, const QString &item);
    void prependItem(const QString &key, const QString &item);
    void clearList();

private slots:
    void upButtonPressed();
    void downButtonPressed();

private:
    void sequenceEntries();
    Ui::ListControlWidget *ui;
};

#endif // LISTCONTROLWIDGET_H
