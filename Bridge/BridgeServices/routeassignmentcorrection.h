#ifndef ROUTEASSIGNMENTCORRECTION_H
#define ROUTEASSIGNMENTCORRECTION_H

#include "Bridge/Greenmile/gmconnection.h"
#include "Bridge/bridgedatabase.h"
#include <QObject>

class RouteAssignmentCorrection : public QObject
{
    Q_OBJECT
public:
    explicit RouteAssignmentCorrection(QObject *parent = nullptr);
    virtual ~RouteAssignmentCorrection();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);

    void finished(const QString &key, const QJsonObject &result);

public slots:
    void CorrectRouteAssignments(const QString &key, const QList<QVariantMap> &argList);

private slots:
    void handleGMDeleteAssignmentResponse(const QString &key, const QJsonValue &response);
    void handleGMUploadAssignmentResponse(const QString &key, const QJsonValue &response);

private:
    void deleteAssignments();
    void uploadAssignments();

    GMConnection *gmDeleteConn_ = new GMConnection(this);
    GMConnection *gmAssignConn_ = new GMConnection(this);
    BridgeDatabase *bridgeDB_   = new BridgeDatabase(this);

    QString currentKey_;
    QVariantMap currentRequest_;
    QSet<QString> activeJobs_;

    QJsonObject routeAssignmentsToCorrect_;
    QJsonObject correctedRouteAssignments_;
    void mergeRouteAssignmentCorrections(const QJsonObject &locations);
    void reset();
};

#endif // ROUTEASSIGNMENTCORRECTION_H
