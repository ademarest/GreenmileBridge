#ifndef LOCATIONGEOCODE_H
#define LOCATIONGEOCODE_H

#include <QObject>

class LocationGeocode : public QObject
{
    Q_OBJECT
public:
    explicit LocationGeocode(QObject *parent = nullptr);

signals:

public slots:
};

#endif // LOCATIONGEOCODE_H