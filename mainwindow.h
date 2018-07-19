#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Bridge/Greenmile/greenmileconfigwidget.h"
#include "Bridge/bridgeprogresswidget.h"
#include "Bridge/MasterRoute/masterroutesheetconfigwidget.h"
#include "Bridge/AS400/as400configwidget.h"
#include "Bridge/MasterRouteData/masterroutesheetdataconfigwidget.h"
#include "Bridge/bridgeconfigwidget.h"

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
    GreenmileConfigWidget           *gmConfig_          = new GreenmileConfigWidget(this);
    BridgeProgressWidget            *bridgeProgress_    = new BridgeProgressWidget(this);
    MasterRouteSheetConfigWidget    *mrsConfig_         = new MasterRouteSheetConfigWidget("mrsconnection.db", this);
    MasterRouteSheetConfigWidget    *dlmrsConfig_       = new MasterRouteSheetConfigWidget("dlmrsconnection.db", this);
    AS400ConfigWidget               *as400Config_       = new AS400ConfigWidget(this);
    MasterRouteSheetDataConfigWidget *mrsDataConfig_    = new MasterRouteSheetDataConfigWidget(this);
    BridgeConfigWidget              *bridgeConfig_      = new BridgeConfigWidget(this);

};

#endif // MAINWINDOW_H
