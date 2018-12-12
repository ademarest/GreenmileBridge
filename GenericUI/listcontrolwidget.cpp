#include "listcontrolwidget.h"
#include "ui_listcontrolwidget.h"

ListControlWidget::ListControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ListControlWidget)
{
    ui->setupUi(this);
    connect(ui->upButton,   &QPushButton::clicked, this, &ListControlWidget::upButtonPressed);
    connect(ui->downButton, &QPushButton::clicked, this, &ListControlWidget::downButtonPressed);
    connect(ui->deleteButton, &QPushButton::clicked, this, &ListControlWidget::deleteButtonPressed);
    connect(ui->resetButton, &QPushButton::clicked, this, &ListControlWidget::resetButtonPressed);
}

ListControlWidget::~ListControlWidget()
{
    delete ui;
}


void ListControlWidget::addItems(const QStringList &items)
{
    originalItems_ = items;
    QStringList spacedItems;
    for(auto item : items)
    {
        spacedItems.append("  " + item);
    }

    ui->listWidget->addItems(spacedItems);
    sequenceEntries();
}

void ListControlWidget::appendItem( const QString &item)
{
    originalItems_.append(item);
    QString spacedItem = "  " + item;
    ui->listWidget->addItem(spacedItem);
    sequenceEntries();
}

void ListControlWidget::prependItem(const QString &item)
{
    originalItems_.prepend(item);
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

void ListControlWidget::deleteButtonPressed()
{
    int idx = ui->listWidget->currentRow();
    delete ui->listWidget->takeItem(idx);
    sequenceEntries();
}

void ListControlWidget::resetButtonPressed()
{
    ui->listWidget->clear();
    addItems(originalItems_);
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
