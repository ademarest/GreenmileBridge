#include "listcontrolwidget.h"
#include "ui_listcontrolwidget.h"

ListControlWidget::ListControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ListControlWidget)
{
    ui->setupUi(this);
    connect(ui->upButton,   &QPushButton::clicked, this, &ListControlWidget::upButtonPressed);
    connect(ui->downButton, &QPushButton::clicked, this, &ListControlWidget::downButtonPressed);
}

ListControlWidget::~ListControlWidget()
{
    delete ui;
}


void ListControlWidget::addItems(const QString &key, const QStringList &items)
{
    qDebug() << key;
    QStringList spacedItems;
    for(auto item : items)
    {
        spacedItems.append("  " + item);
    }

    ui->listWidget->addItems(spacedItems);
    sequenceEntries();
}

void ListControlWidget::appendItem(const QString &key, const QString &item)
{
    qDebug() << key;
    QString spacedItem = "  " + item;
    ui->listWidget->addItem(spacedItem);
    sequenceEntries();
}

void ListControlWidget::prependItem(const QString &key, const QString &item)
{
    qDebug() << key;
    QString spacedItem = "  " + item;
    ui->listWidget->insertItem(0, spacedItem);
    sequenceEntries();
}

void ListControlWidget::upButtonPressed()
{
    int idx = ui->listWidget->currentRow();
    QListWidgetItem *item = ui->listWidget->takeItem(idx);
    ui->listWidget->insertItem(idx-1, item);
    ui->listWidget->setCurrentRow(idx-1);
    sequenceEntries();
}

void ListControlWidget::downButtonPressed()
{
    int idx = ui->listWidget->currentRow();
    QListWidgetItem *item = ui->listWidget->takeItem(idx);
    ui->listWidget->insertItem(idx+1, item);
    ui->listWidget->setCurrentRow(idx+1);
    sequenceEntries();
}

void ListControlWidget::clearList()
{
    ui->listWidget->clear();
}

void ListControlWidget::sequenceEntries()
{
    for(int i = 0; i < ui->listWidget->count(); ++i)
    {
        QListWidgetItem *currentItem = ui->listWidget->item(i);
        QString text = currentItem->text();
        text.remove(0,2);
        text.prepend(QString::number(i+1) + " ");
        currentItem->setText(text);
    }
}
