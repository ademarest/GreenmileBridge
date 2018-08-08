#ifndef LOCATIONUPLOAD_H
#define LOCATIONUPLOAD_H

#include "Bridge/Greenmile/gmconnection.h"
#include "Bridge/bridgedatabase.h"
#include <QObject>

class LocationUpload : public QObject
{
    Q_OBJECT
public:
    explicit LocationUpload(QObject *parent = nullptr);
    //QJsonObject getResults();

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void finished(const QString &key, const QJsonObject &result);

public slots:
    void UploadLocations(const QString &key, const QList<QVariantMap> &argList, const QJsonObject &geocodes);
    void UpdateLocations(const QString &key, const QList<QVariantMap> &argList, const QJsonObject &geocodes);

private slots:
    void handleGMResponse(const QString &key, const QJsonValue &response);

private:
    GMConnection *gmConn_ = new GMConnection(this);
    BridgeDatabase *bridgeDB_ = new BridgeDatabase(this);

    QString currentKey_;
    QVariantMap currentRequest_;
    QSet<QString> activeJobs_;

    QJsonObject locationsToUpload_;
    QJsonObject uploadedLocations_;
    void mergeLocationsToUpload(const QJsonObject &locations);
    void applyGeocodesToLocations(const QJsonObject &geocodes);
    void reset();

};

#endif // LOCATIONUPLOAD_H
