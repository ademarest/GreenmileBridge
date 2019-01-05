#-------------------------------------------------
#
# Project created by QtCreator 2018-06-18T14:15:35
#
#-------------------------------------------------

QT       += core gui sql network networkauth concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GreenmileBridge
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    Bridge/bridge.cpp \
    Bridge/Greenmile/gmconnection.cpp \
    JsonSettings/jsonsettings.cpp \
    Bridge/Greenmile/greenmileconfigwidget.cpp \
    Bridge/bridgeprogresswidget.cpp \
    Bridge/MasterRoute/masterroutesheetconfigwidget.cpp \
    Bridge/MasterRoute/mrsconnection.cpp \
    Bridge/AS400/as400configwidget.cpp \
    Bridge/AS400/as400connection.cpp \
    Bridge/bridgedatabase.cpp \
    Bridge/MasterRouteData/masterroutesheetdataconfigwidget.cpp \
    Bridge/MasterRouteData/mrsdataconnection.cpp \
    Bridge/bridgeconfigwidget.cpp \
    Bridge/GoogleSheets/googlesheetsconnection.cpp \
    Bridge/bridgedatacollector.cpp \
    Bridge/BridgeServices/locationgeocode.cpp \
    Bridge/BridgeServices/locationupload.cpp \
    Bridge/BridgeServices/routeupload.cpp \
    Bridge/BridgeServices/routeassignmentcorrection.cpp \
    Bridge/BridgeServices/routecheck.cpp \
    Bridge/bridgethreadcontroller.cpp \
    LogWriter/logwriter.cpp \
    Bridge/Internet/httpconn.cpp \
    Bridge/Geocoding/censusgeocode.cpp \
    Bridge/Geocoding/arcgisgeocode.cpp \
    GenericUI/listcontrolwidget.cpp \
    Bridge/BridgeServices/gmabstractentity.cpp \
    Bridge/BridgeServices/locationoverridetimewindow.cpp \
    Bridge/BridgeServices/accounttype.cpp \
    Bridge/BridgeServices/servicetimetype.cpp \
    Bridge/BridgeServices/locationtype.cpp

HEADERS += \
        mainwindow.h \
    Bridge/bridge.h \
    Bridge/Greenmile/gmconnection.h \
    JsonSettings/jsonsettings.h \
    Bridge/Greenmile/greenmileconfigwidget.h \
    Bridge/bridgeprogresswidget.h \
    Bridge/MasterRoute/masterroutesheetconfigwidget.h \
    Bridge/MasterRoute/mrsconnection.h \
    Bridge/AS400/as400configwidget.h \
    Bridge/AS400/as400connection.h \
    Bridge/bridgedatabase.h \
    Bridge/MasterRouteData/masterroutesheetdataconfigwidget.h \
    Bridge/MasterRouteData/mrsdataconnection.h \
    Bridge/bridgeconfigwidget.h \
    Bridge/GoogleSheets/googlesheetsconnection.h \
    Bridge/bridgedatacollector.h \
    Bridge/BridgeServices/locationgeocode.h \
    Bridge/BridgeServices/locationupload.h \
    Bridge/BridgeServices/routeupload.h \
    Bridge/BridgeServices/routeassignmentcorrection.h \
    Bridge/BridgeServices/routecheck.h \
    Bridge/bridgethreadcontroller.h \
    LogWriter/logwriter.h \
    Bridge/Internet/httpconn.h \
    Bridge/Geocoding/censusgeocode.h \
    Bridge/Geocoding/arcgisgeocode.h \
    GenericUI/listcontrolwidget.h \
    Bridge/BridgeServices/gmabstractentity.h \
    Bridge/BridgeServices/locationoverridetimewindow.h \
    Bridge/BridgeServices/accounttype.h \
    Bridge/BridgeServices/servicetimetype.h \
    Bridge/BridgeServices/locationtype.h

FORMS += \
        mainwindow.ui \
    Bridge/Greenmile/greenmileconfigwidget.ui \
    Bridge/bridgeprogresswidget.ui \
    Bridge/MasterRoute/masterroutesheetconfigwidget.ui \
    Bridge/AS400/as400configwidget.ui \
    Bridge/MasterRouteData/masterroutesheetdataconfigwidget.ui \
    Bridge/bridgeconfigwidget.ui \
    GenericUI/listcontrolwidget.ui
