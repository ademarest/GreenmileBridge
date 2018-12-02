#ifndef LOCATIONGEOCODE_H
#define LOCATIONGEOCODE_H

#include "Bridge/Greenmile/gmconnection.h"
#include "Bridge/bridgedatabase.h"
#include "Bridge/Geocoding/censusgeocode.h"
#include "Bridge/Geocoding/arcgisgeocode.h"
#include <QObject>

class LocationGeocode : public QObject
{
    Q_OBJECT
public:
    explicit LocationGeocode(QObject *parent = nullptr);
    virtual ~LocationGeocode();
    //QJsonObject getResults();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void progress();
    void finished(const QString &key, const QJsonObject &result);
    void failed(const QString &key, const QString &reason);

public slots:
    void GeocodeLocations(const QString &key, const QList<QVariantMap> &argList, const bool update, const bool fixBadGeocodes);

private slots:
    void handleGMResponse(const QString &key, const QJsonValue &response);
    void handleCensusResponse(const QString &key, const QJsonValue &response);
    void handleArcGISResponse(const QString &key, const QJsonValue &response);
    void acknowledgeFailure(const QString &key, const QString &reason);

private:

    //Maybe make these fails their own class or struct...
    bool failState_ = false;
    QString failReason_;
    QString failKey_;

    GMConnection    *gmConn_        = new GMConnection(this);
    CensusGeocode   *censusConn_    = new CensusGeocode("census.db", this);
    ARCGISGeocode   *arcGISConn_    = new ARCGISGeocode("arcgis.db", this);

    BridgeDatabase *bridgeDB_ = new BridgeDatabase(this);

    QString currentKey_;
    QSet<QString> activeJobs_;
    QVariantMap currentRequest_;

    QJsonObject locationsToGeocode_;
    QJsonObject geocodedLocations_;

    void handleJobCompletion(const QString &key);
    void mergeLocationsToGeocode(const QJsonObject &locations);
    void reset();

    void getLocationsToGeocode(QList<QVariantMap> argList,
                               const bool update = false,
                               const bool fixBadGeocodes = false);

    void startGeocoding(const QString &geocodingService = "arcgis");

    template<class T>
    void dispatchGeocodeRequests(T *geocodingService)
    {
        for(auto key:locationsToGeocode_.keys())
        {
            activeJobs_.insert(key);
            geocodingService->geocodeLocation(key, locationsToGeocode_[key].toObject());
        }
    }
};

#endif // LOCATIONGEOCODE_H
