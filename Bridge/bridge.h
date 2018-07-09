#ifndef BRIDGE_H
#define BRIDGE_H

#include <QObject>
#include <QtConcurrent/QtConcurrent>
#include "Greenmile/gmconnection.h"
#include "MasterRoute/mrsconnection.h"
#include "AS400/as400connection.h"
#include "MasterRouteData/mrsdataconnection.h"
#include "Bridge/bridgedatabase.h"

class Bridge : public QObject
{
    Q_OBJECT
public:
    explicit Bridge(QObject *parent = nullptr);

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);

public slots:
    void startBridge();
    void handleRouteQueryResults(QMap<QString, QVariantList> sqlResults);

private:
    GMConnection *gmConn = new GMConnection(this);
    MRSConnection *mrsConn = new MRSConnection(this);
    AS400 *as400Conn = new AS400(this);
    MRSDataConnection *mrsDataConn = new MRSDataConnection(this);
    BridgeDatabase *bridgeDB = new BridgeDatabase(this);
};

#endif // BRIDGE_H
