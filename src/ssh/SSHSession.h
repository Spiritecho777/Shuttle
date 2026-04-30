#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <QMutex>
#include <atomic>

#include "AuthMethod.h"

#include <libssh2.h>

class SSHSession : public QThread
{
	Q_OBJECT


public:
	SSHSession(const QString& host, int port, const QString& username, const QString& password, const QString& privateKeyPath, int portTunnel, const QString& passphrase = "", QObject* parent = nullptr);

	~SSHSession();

	void writeData(const QByteArray& data);
	void disconnectSession();
	void setAuthMethod(AuthMethod m) { authMethod = m; }
	void resizePty(int cols, int rows);

signals:
	void connected();
	void connectionFailed(const QString& error);
	void dataReceived(const QByteArray& data);
	void disconnected();

protected:
	void run() override;

private:
	QString host;
	int port;
	QString username;
	QString password;
	QString privateKeyPath;
	int portTunnel;
	QString passphrase;

	AuthMethod authMethod{ AuthMethod::Password };

	QMutex m_mutex;

#ifdef _WIN32
	SOCKET sock = INVALID_SOCKET;
#else
	int sock = -1;
#endif
	LIBSSH2_SESSION* session = nullptr;
	LIBSSH2_CHANNEL* channel = nullptr;

	std::atomic<bool> running{ false };
	std::atomic<bool> m_disconnecting{ false };
};