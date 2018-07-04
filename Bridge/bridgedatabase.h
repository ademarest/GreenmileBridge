#ifndef BRIDGEDATABASE_H
#define BRIDGEDATABASE_H

#include <QObject>

class BridgeDatabase : public QObject
{
    Q_OBJECT
public:
    explicit BridgeDatabase(QObject *parent = nullptr);

signals:

public slots:

private:
    void makeAS400RouteQueryTable();

};

#endif // BRIDGEDATABASE_H
