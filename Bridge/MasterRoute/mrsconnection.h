#ifndef MRSCONNECTION_H
#define MRSCONNECTION_H

#include <QObject>

class MRSConnection : public QObject
{
    Q_OBJECT
public:
    explicit MRSConnection(QObject *parent = nullptr);

signals:

public slots:
};

#endif // MRSCONNECTION_H