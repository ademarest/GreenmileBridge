#ifndef OAUTH2BROKER_H
#define OAUTH2BROKER_H

#include <QObject>

class OAuth2Broker : public QObject
{
    Q_OBJECT
public:
    explicit OAuth2Broker(QObject *parent = nullptr);

signals:

public slots:
};

#endif // OAUTH2BROKER_H