#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->greenmileConfigGrid->addWidget(gmConfig_);
    ui->bridgeProgressGridLayout->addWidget(bridgeProgress_);
}

MainWindow::~MainWindow()
{
    delete ui;
}
