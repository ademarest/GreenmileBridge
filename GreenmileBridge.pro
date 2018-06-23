#-------------------------------------------------
#
# Project created by QtCreator 2018-06-18T14:15:35
#
#-------------------------------------------------

QT       += core gui sql network

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
    Bridge/MasterRoute/mrsconnection.cpp

HEADERS += \
        mainwindow.h \
    Bridge/bridge.h \
    Bridge/Greenmile/gmconnection.h \
    JsonSettings/jsonsettings.h \
    Bridge/Greenmile/greenmileconfigwidget.h \
    Bridge/bridgeprogresswidget.h \
    Bridge/MasterRoute/masterroutesheetconfigwidget.h \
    Bridge/MasterRoute/mrsconnection.h

FORMS += \
        mainwindow.ui \
    Bridge/Greenmile/greenmileconfigwidget.ui \
    Bridge/bridgeprogresswidget.ui \
    Bridge/MasterRoute/masterroutesheetconfigwidget.ui
