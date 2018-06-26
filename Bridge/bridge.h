#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include "Greenmile/gmconnection.h"
#include "MasterRoute/mrsconnection.h"

class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject *parent = nullptr);
    bool uploadRoutes();

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);

public slots:
    void startBridge();
    void stopBridge();
    void handleRouteKeysForDate(QJsonArray routeArray);
    void handleLocationKeys(QJsonArray locationArray);

private:
    GMConnection *gmConn = new GMConnection(this);
    MRSConnection *mrsConn = new MRSConnection(this);
    bool bridgeRunStatus_ = false;
    void bridgeLoop();
};

#endif // BRIDGE_H
