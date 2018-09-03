#ifndef LOGWRITER_H
#define LOGWRITER_H

#include <QObject>
#include <QtCore>

class LogWriter : public QObject
{
    Q_OBJECT
public:
    explicit LogWriter(QObject *parent = Q_NULLPTR);
    explicit LogWriter(const QString &logPath, QObject *parent = Q_NULLPTR);
    virtual ~LogWriter();

signals:
    void statusMessage(const QString &status);
    void debugMessage(const QString &debug);
    void errorMessage(const QString &error);

public slots:
    bool writeLogEntry(QString entry);

private:
    QString logPath_;
};

#endif // LOGWRITER_H
