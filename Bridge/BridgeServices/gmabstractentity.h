#ifndef GMABSTRACTENTITY_H
#define GMABSTRACTENTITY_H

#include "Bridge/bridgedatacollector.h"
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
    void finished(const QString &key, const QMap<QString, QJsonObject> result);
    void failed(const QString &key, const QString &reason);

protected slots:
    virtual void handleGMResponse(const QString &key, QJsonValue response);
    virtual void handleFailure(const QString &key, const QString &reason);
    virtual void processEntities(const QString &key, const QList<QVariantMap> &argList);

protected:
    bool            failState_ = false;
    QString         failReason_ = QString();
    QString         failKey_ = QString();

    GMConnection        *gmConn_    = new GMConnection(this);
    BridgeDatabase      *bridgeDB_  = new BridgeDatabase(this);
    BridgeDataCollector *bridgeDC_  = new BridgeDataCollector(this);

    QString         currentKey_;
    QVariantMap     currentRequest_;
    QSet<QString>   activeJobs_;

    QMap<QString, QJsonObject> entitiesToProcess_;
    QMap<QString, QJsonObject> entitiesProcessed_;

    QMap<QString, std::function<QJsonObject (BridgeDatabase *, QVariantMap)>>   databaseFuncs_;
    QMap<QString, std::function<QJsonValue(QJsonValue)>>                        preprocessFuncs_;
    QMap<QString, std::function<void(GMConnection*, QString, QJsonObject)>>     internetFuncs_;
    QMap<QString, std::function<QJsonValue(QJsonValue)>>                        postProcessFuncs_;
    QMap<QString, std::function<void(BridgeDataCollector*, QJsonArray)>>        bridgeDataCollectorFuncs_;

    static QJsonObject mergeEntities(QJsonObject initialData, const QJsonObject &additionalData);
    static QJsonObject prefixEntityKeys(const QString &prefix, const QJsonObject &entity);

    void generateJobKeys();
    void executeDatabaseFuncs(const QList<QVariantMap> &argList);
    void executePreProcessFuncs();
    void executeInternetFuncs();
    void executePostProcessFuncs(const QString &key, const QString &operationKey);
    void executeInsertResponsesToDB();

    virtual void reset();
};

#endif // GMABSTRACTENTITY_H
