#include "SSHSessionManager.h"

SSHSessionManager::SSHSessionManager(QObject* parent) : QObject(parent) {}

SSHSessionManager::~SSHSessionManager() { closeAll(); }

int SSHSessionManager::openSession(const SessionProfile& profile)
{
    int id = m_nextId++;
    auto* s = new SSHSession(
        profile.host, 
        profile.port, 
        profile.username,
		profile.password,
        profile.privateKeyPath, 
        profile.passphrase, 
        this
    );

    s->setAuthMethod(profile.privateKeyPath.isEmpty() ? AuthMethod::Password : AuthMethod::PublicKey);

	connect(s, &QThread::finished, s, &QObject::deleteLater);
    connect(s, &SSHSession::connected, this, [this, id]() { emit sessionConnected(id); });
    connect(s, &SSHSession::connectionFailed, this, [this, id](const QString& e) { emit sessionFailed(id, e); });
    connect(s, &SSHSession::disconnected, this, [this, id]() { emit sessionDisconnected(id); });
    connect(s, &SSHSession::dataReceived, this, [this, id](const QByteArray& d) { emit sessionData(id, d); });

    m_sessions[id] = s;
    s->start();
    return id;
}

void SSHSessionManager::closeSession(int id)
{
    if (auto* s = m_sessions.value(id, nullptr)) {
        s->disconnectSession();
        s->wait(3000);
        s->deleteLater();
        m_sessions.remove(id);
    }
}

void SSHSessionManager::closeAll()
{
    for (int id : m_sessions.keys())
        closeSession(id);
}

SSHSession* SSHSessionManager::session(int id) const { return m_sessions.value(id, nullptr); }
QList<int>  SSHSessionManager::sessionIds() const { return m_sessions.keys(); }