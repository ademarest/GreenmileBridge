#ifndef SERVICETIMETYPE_H
#define SERVICETIMETYPE_H

#include "gmabstractentity.h"

class ServiceTimeType : public GMAbstractEntity
{
    Q_OBJECT
public:
    explicit ServiceTimeType(QObject *parent = nullptr);

public slots:
    void processAccountTypes(const QString &key, const QList<QVariantMap> &argList);

};

#endif // SERVICETIMETYPE_H
