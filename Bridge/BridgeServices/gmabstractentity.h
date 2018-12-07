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
    void finished(const QString &key,
                  const QJsonObject &uploadedData,
                  const QJsonObject &updatedData,
                  const QJsonObject &deletedData);
    void failed(const QString &key, const QString &reason);

protected slots:
    virtual void handleGMResponse(const QString &key, const QJsonValue &response);
    virtual void handleFailure(const QString &key, const QString &reason);

    virtual void processEntities(const QString &key,
                                 const QList<QVariantMap> &argList,
                                 std::function<QJsonObject(BridgeDatabase*, QVariantMap)> getUploadsFromDatabaseFunc,
                                 std::function<QJsonObject(BridgeDatabase*, QVariantMap)> getUpdatesFromDatabaseFunc,
                                 std::function<QJsonObject(BridgeDatabase*, QVariantMap)> getDeletesFromDatabaseFunc,
                                 std::function<void(GMConnection*, QString, QJsonObject)> uploadFunc,
                                 std::function<void(GMConnection*, QString, QJsonObject)> updateFunc,
                                 std::function<void(GMConnection*, QString, QJsonObject)> deleteFunc);

protected:
    bool            failState_ = false;
    QString         failReason_ = QString();
    QString         failKey_ = QString();

    GMConnection    *gmConn_ = new GMConnection(this);
    BridgeDatabase  *bridgeDB_ = new BridgeDatabase(this);

    QString         currentKey_;
    QVariantMap     currentRequst_;
    QSet<QString>   activeJobs_;

    QJsonObject     entitiesToUpload_;
    QJsonObject     entitiesToUpdate_;
    QJsonObject     entitiesToDelete_;

    QJsonObject     entitiesUploaded_;
    QJsonObject     entitiesUpdated_;
    QJsonObject     entitiesDeleted_;

    static QJsonObject mergeEntities(QJsonObject initialData, const QJsonObject &additionalData);
    static QJsonObject prefixEntityKeys(const QString &prefix, const QJsonObject &entity);

    virtual void reset();
};

#endif // GMABSTRACTENTITY_H
