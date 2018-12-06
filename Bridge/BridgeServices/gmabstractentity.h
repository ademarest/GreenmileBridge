#ifndef GMABSTRACTENTITY_H
#define GMABSTRACTENTITY_H

#include "Bridge/Greenmile/gmconnection.h"
#include "Bridge/bridgedatabase.h"
#include <QObject>

class GMAbstractEntity : public QObject
{
    Q_OBJECT
public:
    explicit GMAbstractEntity(QObject *parent = nullptr);
    virtual ~GMAbstractEntity();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void finished(const QString &key, const QJsonObject &result);
    void failed(const QString &key, const QString &reason);

protected slots:
    virtual void handleGMResponse(const QString &key, const QJsonValue &response);
    virtual void handleFailure(const QString &key, const QString &reason);

protected:
    bool            failState_ = false;
    QString         failReason_ = QString();
    QString         failKey_ = QString();

    GMConnection    *gmConn_ = new GMConnection(this);
    BridgeDatabase  *bridgeDB_ = new BridgeDatabase(this);

    QString         currentKey_;
    QVariantMap     currentRequst_;
    QSet<QString>   activeJobs_;

    QJsonObject     entitiesToProcess_;
    QJsonObject     entitiesProcessed_;

    void mergeEntities(const QJsonObject &jObj);

    virtual void reset();
};

#endif // GMABSTRACTENTITY_H
