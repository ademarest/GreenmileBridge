#ifndef LOCATIONTYPE_H
#define LOCATIONTYPE_H

#include "gmabstractentity.h"

class LocationType : public GMAbstractEntity
{
Q_OBJECT
public:
    explicit LocationType(QObject *parent = nullptr);
public slots:
    void processLocationTypes(const QString &key, const QList<QVariantMap> &argList);
};

#endif // LOCATIONTYPE_H
