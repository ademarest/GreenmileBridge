#ifndef HTTPCONN_H
#define HTTPCONN_H

#include "JsonSettings/jsonsettings.h"
#include <QObject>
#include <QtNetwork>
#include <unistd.h>

class HTTPConn : public QObject
{
    Q_OBJECT
public:
    explicit HTTPConn(const QString &datbaseName, QObject *parent = nullptr);
    explicit HTTPConn(const QString &databaseName,
                      const QString &serverAddress,
                      const QString &username,
                      const QString &password,
                      const QStringList &headers,
                      const int connectionFreqMS,
                      const int maxActivceConnections,
                      QObject *parent = nullptr);

    virtual ~HTTPConn();

    void setServerAddress(const QString &serverAddress);
    void setUsername(const QString &username);
    void setPassword(const QString &password);
    void addHeader(const QString &headerName, const QString &headerValue);
    bool isProcessingNetworkRequests();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);
    void downloadProgess(qint64 bytesReceived, qint64 bytesTotal);
    void gmNetworkResponse(const QString &key, const QJsonValue &obj);

protected:
    void addToConnectionQueue(const QNetworkAccessManager::Operation requestType,
                              const QString &requestKey,
                              const QString &serverAddrTail,
                              const QByteArray &data = QByteArray(),
                              const QString &customOperation = QString());

    QString dbPath_         = qApp->applicationDirPath()
            + "/" + QUuid::createUuid().toString() + ".db";

    JsonSettings *settings_ = new JsonSettings(this);
    QJsonObject jsonSettings_ {{"serverAddress",        QJsonValue("https://example.com")},
                               {"username",             QJsonValue("username")},
                               {"password",             QJsonValue("password")},
                               {"headers",              QJsonArray()},
                               {"requestTimeoutSec",    QJsonValue(40)},
                               {"connectionFreqMS",     QJsonValue(100)},
                               {"maxActiveConnections", QJsonValue(10)}};

private slots:
    void prepConnectionQueue();
    void setReadyForNextConnection();
    void processConnectionQueue();

    void handleNetworkReply(QNetworkReply *reply);
    void startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal);
    void requestTimedOut();

private:
    bool usingTempDB_               = true;
    bool readyForNextConnection_    = true;
    int numberOfActiveConnections_  = 0;
    int checkQueueIntervalMS_       = 1000;
    QTimer *checkQueueTimer_        = new QTimer(this);

    QNetworkAccessManager *qnam_    = new QNetworkAccessManager(this);
    QTimer *routeKeyResponseTimer_  = new QTimer(this);
    QTimer *connectionFrequencyTimer_  = new QTimer(this);

    QNetworkRequest makeNetworkRequest(const QString &serverAddrTail);

    //Test code loading up all network information
    QMap<QString, QNetworkAccessManager*> networkManagers_;
    QMap<QString, QNetworkReply*> networkReplies_;
    QMap<QString, QTimer*> networkTimers_;
    QMap<QString, QBuffer*> networkBuffers_;
    QSet<QString> networkRequestsInProgress_;
    QQueue<QVariantMap> connectionQueue_;
};

#endif // HTTPCONN_H
