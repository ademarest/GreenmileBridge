#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QMainWindow::showMaximized();
    ui->greenmileConfigGrid->addWidget(gmConfig_);
    ui->bridgeProgressGridLayout->addWidget(bridgeProgress_);
    ui->as400ConfigGrid->addWidget(as400Config_);
    ui->mrsDataConfigGrid->addWidget(mrsDataConfig_);
    ui->bridgeConfigGrid->addWidget(bridgeConfig_);
    ui->scheduleSheetsGrid->addWidget(schedConfig_);
    this->setWindowTitle("Greenmile API Bridge");
}

MainWindow::~MainWindow()
{
    delete ui;
}
