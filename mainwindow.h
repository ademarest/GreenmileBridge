#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Bridge/Greenmile/greenmileconfigwidget.h"
#include "Bridge/bridgeprogresswidget.h"
#include "Bridge/MasterRoute/masterroutesheetconfigwidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    GreenmileConfigWidget *gmConfig_ = new GreenmileConfigWidget(this);
    BridgeProgressWidget *bridgeProgress_ = new BridgeProgressWidget(this);
    MasterRouteSheetConfigWidget *mrsConfig_ = new MasterRouteSheetConfigWidget(this);
};

#endif // MAINWINDOW_H
