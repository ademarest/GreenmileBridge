#ifndef ROUTECHECK_H
#define ROUTECHECK_H

#include "Bridge/Greenmile/gmconnection.h"
#include "Bridge/bridgedatabase.h"
#include <QObject>

class RouteCheck : public QObject
{
    Q_OBJECT
public:
    explicit RouteCheck(QObject *parent = nullptr);
    virtual ~RouteCheck();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void finished(const QString &key, const QJsonObject &result);

public slots:
    void deleteIncorrectRoutes(const QString &key, const QList<QVariantMap> &argList);

private slots:
    void handleGMResponse(const QString &key, const QJsonValue &response);

private:
    GMConnection *gmConn_ = new GMConnection(this);
    BridgeDatabase *bridgeDB_ = new BridgeDatabase(this);

    QString currentKey_;
    QVariantMap currentRequest_;
    QSet<QString> activeJobs_;

    QJsonObject routesToDelete_;
    QJsonObject deletedRoutes_;
    void mergeRoutesToDelete(const QJsonObject &routeIDs);
    void reset();
};

#endif // ROUTECHECK_H
