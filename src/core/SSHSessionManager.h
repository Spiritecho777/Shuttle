#pragma once

#include <QObject>
#include <QMap>
#include "SSHSession.h"

struct SessionProfile {
    QString name;
    QString host;
    int     port = 22;
    QString username;
	QString password;
    QString privateKeyPath;
    QString passphrase;
};

class SSHSessionManager : public QObject
{
    Q_OBJECT

public:
    explicit SSHSessionManager(QObject* parent = nullptr);
    ~SSHSessionManager();

    // Retourne l'id de session (-1 si échec immédiat)
    int openSession(const SessionProfile& profile);
    void closeSession(int id);
    void closeAll();

    SSHSession* session(int id) const;
    QList<int>  sessionIds() const;

signals:
    void sessionConnected(int id);
    void sessionFailed(int id, const QString& error);
    void sessionDisconnected(int id);
    void sessionData(int id, const QByteArray& data);

private:
    QMap<int, SSHSession*> m_sessions;
    int m_nextId = 0;
};