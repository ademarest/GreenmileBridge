#ifndef LOCATIONOVERRIDETIMEWINDOW_H
#define LOCATIONOVERRIDETIMEWINDOW_H

#include "gmabstractentity.h"

class LocationOverrideTimeWindow : public GMAbstractEntity
{
    Q_OBJECT
public:
    explicit LocationOverrideTimeWindow(QObject *parent = nullptr);

public slots:
    void processLocationOverrideTimeWindows(const QString &key, const QList<QVariantMap> &argList);
};

#endif // LOCATIONOVERRIDETIMEWINDOW_H
