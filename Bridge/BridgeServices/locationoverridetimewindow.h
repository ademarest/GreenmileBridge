#ifndef LOCATIONOVERRIDETIMEWINDOW_H
#define LOCATIONOVERRIDETIMEWINDOW_H

#include "gmabstractentity.h"
#include <QObject>

class LocationOverrideTimeWindow : public GMAbstractEntity
{
    Q_OBJECT
public:
    explicit LocationOverrideTimeWindow(QObject *parent = nullptr);

signals:

public slots:
    void UploadLocationOverrideTimeWindows();
    void UpdateLocationOverrideTimeWindows();
    void DeleteLocationOverrideTimeWindows();

};

#endif // LOCATIONOVERRIDETIMEWINDOW_H
