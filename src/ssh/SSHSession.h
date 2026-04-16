#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <atomic>

#include <libssh2.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

enum class AuthMethod {
	Password,
	PublicKey
};

class SSHSession : public QThread
{
	Q_OBJECT


public:
	SSHSession(const QString& host, int port, const QString& username, const QString& password, const QString& privateKeyPath, const QString& passphrase = "", QObject* parent = nullptr);

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
	QString passphrase;

	AuthMethod authMethod{ AuthMethod::Password };

#ifdef _WIN32
	SOCKET sock = INVALID_SOCKET;
#else
	int sock = -1;
#endif
	LIBSSH2_SESSION* session = nullptr;
	LIBSSH2_CHANNEL* channel = nullptr;

	std::atomic<bool> running{ false };
};