#ifndef ACCOUNTTYPE_H
#define ACCOUNTTYPE_H

#include "gmabstractentity.h"

class AccountType : public GMAbstractEntity
{
    Q_OBJECT
public:
    explicit AccountType(QObject *parent = nullptr);
public slots:
    void processAccountTypes(const QString &key, const QList<QVariantMap> &argList);
};

#endif // ACCOUNTTYPE_H
