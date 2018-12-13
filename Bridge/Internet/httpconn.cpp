#include "httpconn.h"

HTTPConn::HTTPConn(const QString &databaseName, QObject *parent) : QObject(parent)
{
    if(!databaseName.isEmpty())
    {
        usingTempDB_ = false;
        dbPath_ = qApp->applicationDirPath() + "/" + databaseName;
    }
    else
    {
        emit debugMessage("Warning, no database name has been set. Using a temp db.");
    }

    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    checkQueueTimer_->start(checkQueueIntervalMS_);
    connect(checkQueueTimer_, &QTimer::timeout, this, &HTTPConn::prepConnectionQueue);
    connect(connectionFrequencyTimer_, &QTimer::timeout, this, &HTTPConn::processConnectionQueue);
}

HTTPConn::HTTPConn(const QString &databaseName,
                   const QString &serverAddress,
                   const QString &username,
                   const QString &password,
                   const QStringList &headers,
                   const int connectionFreqMS,
                   const int maxActiveConnections,
                   QObject *parent) : QObject(parent)
{
    QJsonArray jsonHeaders;
    for(auto header:headers)
    {
        jsonHeaders.append(QJsonValue(header));
    }

    if(!databaseName.isEmpty())
    {
        usingTempDB_ = false;
        dbPath_ = qApp->applicationDirPath() + "/" + databaseName;
    }

    jsonSettings_["serverAddress"]          = QJsonValue(serverAddress);
    jsonSettings_["username"]               = QJsonValue(username);
    jsonSettings_["password"]               = QJsonValue(password);
    jsonSettings_["headers"]                = QJsonValue(jsonHeaders);
    jsonSettings_["connectionFreqMS"]       = QJsonValue(connectionFreqMS);
    jsonSettings_["maxActiveConnections"]   = QJsonValue(maxActiveConnections);
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
    checkQueueTimer_->start(checkQueueIntervalMS_);
    connect(checkQueueTimer_, &QTimer::timeout, this, &HTTPConn::processConnectionQueue);
}

HTTPConn::~HTTPConn()
{
    qDebug() << "~HTTP";
    if(usingTempDB_)
    {
        QFile db(dbPath_);
        db.remove();
    }
}

void HTTPConn::setServerAddress(const QString &serverAddress)
{
    jsonSettings_["serverAddress"] = QJsonValue(serverAddress);
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
}

void HTTPConn::setUsername(const QString &username)
{
    jsonSettings_["username"] = QJsonValue(username);
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
}

void HTTPConn::setPassword(const QString &password)
{
    jsonSettings_["password"] = QJsonValue(password);
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
}

void HTTPConn::addHeader(const QString &headerName, const QString &headerValue)
{
    if(headerName.isEmpty() || headerValue.isEmpty())
    {
        emit errorMessage("Cannot add header. "
                          "Header name or header value is empty."
                          "Header name: " + headerName
                          + ". Header value: " + headerValue + ".");
        return;
    }

    QJsonArray headers;
    QJsonObject headerObj;
    headerObj["headerName"]  = QJsonValue(headerName);
    headerObj["headerValue"] = QJsonValue(headerValue);
    headers.append(QJsonValue(headerObj));
    jsonSettings_["headers"] = QJsonValue(headers);
    settings_->saveSettings(QFile(dbPath_), jsonSettings_);
}

bool HTTPConn::isProcessingNetworkRequests()
{
    if(networkRequestsInProgress_.isEmpty() &&  connectionQueue_.isEmpty())
        return false;
    else
        return true;
}


void HTTPConn::addToConnectionQueue(const QNetworkAccessManager::Operation requestType,
                                        const QString &requestKey,
                                        const QString &serverAddrTail,
                                        const QByteArray &data,
                                        const QString &customOperation)
{
    QVariantMap requestInfo {{"requestType", requestType},
                             {"requestKey", requestKey},
                             {"serverAddrTail", serverAddrTail},
                             {"data", data},
                             {"customOperation", customOperation}};

    connectionQueue_.enqueue(requestInfo);
}

void HTTPConn::prepConnectionQueue()
{
    jsonSettings_ = settings_->loadSettings(QFile(dbPath_), jsonSettings_);
    connectionFrequencyTimer_->start(jsonSettings_["connectionFreqMS"].toInt());
    processConnectionQueue();
}

void HTTPConn::setReadyForNextConnection()
{
    readyForNextConnection_ = true;
    processConnectionQueue();
}

void HTTPConn::processConnectionQueue()
{
    if(!readyForNextConnection_)
        return;

    if(numberOfActiveConnections_ > jsonSettings_["maxActiveConnections"].toInt())
    {
        emit debugMessage("Too many connections, more than "
                          + QString::number(jsonSettings_["maxActiveConnections"].toInt()));

        qDebug() << "Too many connections! There's more than " << jsonSettings_["maxActiveConnections"].toInt();
        return;
    }

    if(connectionQueue_.isEmpty())
    {
//        Gets spammy real fast.
//
//        emit debugMessage("HTTPConn::processConnectionQueue(): "
//                          "Connection queue is empty. "
//                          "Will not attempt to dequeue.");

        connectionFrequencyTimer_->stop();
        readyForNextConnection_ = true;
        return;
    }

    qDebug() << "START:HTTPConn::processConnectionQueue";
    emit debugMessage("START:HTTPConn::processConnectionQueue");

    QVariantMap requestMap      = connectionQueue_.dequeue();
    int requestType             = requestMap["requestType"].toInt();
    QString requestKey          = requestMap["requestKey"].toString();
    QString serverAddrTail      = requestMap["serverAddrTail"].toString();
    QByteArray data             = requestMap["data"].toByteArray();
    QString customOperation   = requestMap["customOperation"].toByteArray();

    emit statusMessage("START:HTTPConn::" + requestKey);

    if(networkRequestsInProgress_.contains(requestKey))
    {
        emit errorMessage("Net request " + requestKey + " already in progress. The request will retry when the current request has completed.");
        qDebug() << "Net request " << requestKey << " already in progress. The request will retry when the current request has completed.";
        connectionQueue_.enqueue(requestMap);
        return;
    }
    QNetworkRequest request = makeNetworkRequest(serverAddrTail);

    if(data.isEmpty())
    {
        networkBuffers_[requestKey] = Q_NULLPTR;
    }
    else
    {
        networkBuffers_[requestKey] =  new QBuffer(this);
        networkBuffers_[requestKey]->open((QBuffer::ReadWrite));
        networkBuffers_[requestKey]->write(data);
        networkBuffers_[requestKey]->seek(0);
    }

    networkTimers_[requestKey] = new QTimer(this);
    networkTimers_[requestKey]->setObjectName(requestKey);

    networkManagers_[requestKey] = new QNetworkAccessManager(this);
    networkManagers_[requestKey]->setObjectName(requestKey);

    switch(requestType)
    {
        case QNetworkAccessManager::Operation::HeadOperation :
            emit debugMessage("HeadOperation not implemented yet.");
            break;
        case QNetworkAccessManager::Operation::GetOperation :
            emit debugMessage("Get request:" + requestKey);
            networkReplies_[requestKey] = networkManagers_[requestKey]->get(request);
            break;
        case QNetworkAccessManager::Operation::PutOperation :
            emit debugMessage("PUT request:" + requestKey);
            networkReplies_[requestKey] = networkManagers_[requestKey]->put(request, data);
            break;
        case QNetworkAccessManager::Operation::PostOperation :
            emit debugMessage("POST request:" + requestKey);
            networkReplies_[requestKey] = networkManagers_[requestKey]->post(request,data);
            break;
        case QNetworkAccessManager::Operation::DeleteOperation :
            emit debugMessage("DELETE request:" + requestKey);
            networkReplies_[requestKey] = networkManagers_[requestKey]->deleteResource(request);
            break;
        case QNetworkAccessManager::Operation::CustomOperation :
            emit debugMessage("CUSTOM request:" + requestKey);
            networkReplies_[requestKey] = networkManagers_[requestKey]->sendCustomRequest(request, customOperation.toLatin1(), networkBuffers_[requestKey]);
            break;
        case QNetworkAccessManager::Operation::UnknownOperation :
            emit debugMessage("Unknown request not impletmented.");
            emit debugMessage("UnknownOperation not implemented yet.");
            break;
    }

    networkReplies_[requestKey]->setObjectName(requestKey);

    connect(networkReplies_ [requestKey],   &QNetworkReply::downloadProgress,   this, &HTTPConn::downloadProgess);
    connect(networkReplies_ [requestKey],   &QNetworkReply::downloadProgress,   this, &HTTPConn::startNetworkTimer);
    connect(networkManagers_[requestKey],   &QNetworkAccessManager::finished,   this, &HTTPConn::handleNetworkReply);
    connect(networkTimers_  [requestKey],   &QTimer::timeout,                   this, &HTTPConn::requestTimedOut);

    networkRequestsInProgress_.insert(requestKey);
    networkTimers_[requestKey]->stop();
    networkTimers_[requestKey]->start(jsonSettings_["requestTimeoutSec"].toInt() * 1000);
    ++numberOfActiveConnections_;

    qDebug() << "END:HTTPConn::processConnectionQueue";
    emit debugMessage("END:HTTPConn::processConnectionQueue");
}

void HTTPConn::handleNetworkReply(QNetworkReply *reply)
{
    qDebug() << "START:HTTPConn::handleNetworkReply";
    emit debugMessage("START:HTTPConn::handleNetworkReply");
    bool hasErrors = false;
    QString key = reply->objectName();
    QByteArray rawData;
    QJsonArray json;
    QJsonObject jObj;
    QJsonDocument jDoc;
    if(reply->error() != QNetworkReply::NoError)
    {
        hasErrors = true;
        emit errorMessage(key + " finished with errors. " + reply->errorString());
        emit errorMessage(key + " server response was " + reply->readAll());
        emit failed(key, reply->errorString());
        qDebug() << reply->error();
    }
    if(reply->isOpen())
    {
        if(hasErrors)
        {
            emit statusMessage(key + " finished with errors.");
        }
        else
        {
            emit statusMessage(key + " finished without errors.");
        }
        rawData = reply->readAll();
        qDebug() << "http raw data... " << rawData;
        jDoc = QJsonDocument::fromJson(rawData);
    }
    networkRequestsInProgress_.remove(key);
    networkTimers_[key]->stop();
    networkTimers_[key]->deleteLater();
    networkManagers_[key]->deleteLater();
    networkReplies_[key]->deleteLater();
    if(networkBuffers_[key])
    {
        networkBuffers_[key]->deleteLater();
    }
    --numberOfActiveConnections_;


    if(jDoc.isArray())
    {
        qDebug() << "GM Response for " + key + " was an array.";
        json = jDoc.array();
        emit networkResponse(key, json);
    }
    else if(jDoc.isObject())
    {
        qDebug() << "GM Response for " + key + " was an object.";
        jObj = jDoc.object();
        emit networkResponse(key, jObj);
    }
    else
    {
        qDebug() << "GM Response for " + key + " was not valid json.";
        qDebug() << "Raw data is " << rawData;
        emit networkResponse(key, QJsonObject());
    }

    qDebug() << "END:HTTPConn::handleNetworkReply";
}

void HTTPConn::startNetworkTimer(qint64 bytesReceived, qint64 bytesTotal)
{
    //handle compiler warning
    qint64 br = bytesReceived;
    br += br;

    QString key = sender()->objectName();
    QTimer* timer = networkTimers_[key];

    //bytesTotal == 0 means the request was aborted.
    if(bytesTotal == 0)
    {
        emit statusMessage(" network request was null or stopped for " + key);
        timer->stop();
        return;
    }

    timer->stop();
    timer->start(jsonSettings_["requestTimeoutSec"].toInt() * 1000);
}

void HTTPConn::requestTimedOut()
{
    QString key = sender()->objectName();
    emit statusMessage("Network request " + key + " has timed out.");
    emit statusMessage("Aborting network call " + key + ".");
    emit failed(key, "Request timed out.");
    //Calling abort also emits finished.
    networkReplies_[key]->abort();
}

QNetworkRequest HTTPConn::makeNetworkRequest(const QString &serverAddrTail)
{
    qDebug() << "START: HTTPConn::makeNetworkRequest";
    QNetworkRequest request;
    QString serverAddress   = jsonSettings_["serverAddress"].toString() + serverAddrTail;
    QString username        = jsonSettings_["username"].toString();
    QString password        = jsonSettings_["password"].toString();

    qDebug() << username << password << serverAddress << jsonSettings_["headers"].toArray();
    request.setUrl(QUrl(serverAddress));
    qDebug() << serverAddress;
    if(!username.isEmpty() || !password.isEmpty())
    {
        QString concatenated = username + ":" + password;
        QByteArray data = concatenated.toLocal8Bit().toBase64();
        QString headerData = "Basic " + data;
        request.setRawHeader("Authorization", headerData.toLocal8Bit());
    }

    for(auto reqJson:jsonSettings_["headers"].toArray())
    {
        QJsonObject reqJsonObj = reqJson.toObject();
        request.setRawHeader(reqJsonObj["headerName"].toString().toUtf8(),
                            reqJsonObj["headerValue"].toString().toUtf8());
    }

    qDebug() << "RHL" << request.rawHeaderList();

    qDebug() << "END: HTTPConn::makeNetworkRequest";
    return request;
}
