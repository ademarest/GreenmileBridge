#ifndef GMCONNECTION_H
#define GMCONNECTION_H

#include "JsonSettings/jsonsettings.h"
#include <QObject>
#include <QtNetwork>

class GMConnection : public QObject
{
    Q_OBJECT
public:
    explicit GMConnection(QObject *parent = nullptr);
    explicit GMConnection(  const QString &serverAddress,
                            const QString &username,
                            const QString &password,
                            QObject *parent = nullptr);

    void setServerAddress(const QString &serverAddress);
    void setUsername(const QString &username);
    void setPassword(const QString &password);

    QString getServerAddress() const;
    QString getUsername() const;

    QJsonArray getRouteKeysForDate(const QDate &date);

signals:
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void statusMessage(const QString &status);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);

public slots:

private:
    QString dbPath_                 = qApp->applicationDirPath() + "/gmconnection.db";
    JsonSettings *settings_         = new JsonSettings(this);
    QJsonObject jsonSettings_ {{"serverAddress", QJsonValue("https://charliesproduce.greenmile.com")},
                               {"username",      QJsonValue("username")},
                               {"password",      QJsonValue("password")}};
    QNetworkAccessManager *qnam_    = new QNetworkAccessManager(this);


};

#endif // GMCONNECTION_H