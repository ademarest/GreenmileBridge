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
    void getRouteKeysFromGMForDate(const QDate &date);

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);
    void routeKeysForDate(QJsonArray array);

public slots:
    void startBridge();
    void handleRouteKeysForDate(QJsonArray routeArray);

private:
    GMConnection *gmConn = new GMConnection();
};

#endif // BRIDGE_H
