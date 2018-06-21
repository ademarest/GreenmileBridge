#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include "Greenmile/gmconnection.h"

class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject *parent = nullptr);
    bool uploadRoutes();

signals:

public slots:

private:
    GMConnection *gmConn = new GMConnection();
};

#endif // BRIDGE_H
