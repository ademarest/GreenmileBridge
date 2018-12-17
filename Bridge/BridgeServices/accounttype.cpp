#include "accounttype.h"

AccountType::AccountType(QObject *parent) : GMAbstractEntity(parent)
{

}

void AccountType::processAccountTypes(const QString &key, const QList<QVariantMap> &argList)
{
    databaseFuncs_["upload"]    = &BridgeDatabase::getAccountTypesToUpload;
    internetFuncs_["upload"]    = &GMConnection::uploadAccountType;
    processEntities(key, argList);
}
