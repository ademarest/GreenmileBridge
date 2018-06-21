#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->greenmileConfigGrid->addWidget(gmConfig_);
    ui->bridgeProgressGridLayout->addWidget(bridgeProgress_);
    connect(gmConfig_, &GreenmileConfigWidget::downloadProgess, bridgeProgress_, &BridgeProgressWidget::updateProgressBarStatus);
    connect(gmConfig_, &GreenmileConfigWidget::statusMessage, bridgeProgress_, &BridgeProgressWidget::writeMessageTextWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}
