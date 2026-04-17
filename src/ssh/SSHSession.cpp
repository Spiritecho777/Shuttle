#include "SSHSession.h"

#include <QDebug>
#include <QFile>
#include <cstring>

#include <libssh2.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

SSHSession::SSHSession(const QString& host, int port, const QString& username, const QString& password, const QString& privateKeyPath, const QString& passphrase, QObject* parent)
	: QThread(parent), host(host), port(port), username(username), password(password), privateKeyPath(privateKeyPath), passphrase(passphrase)
{
}

SSHSession::~SSHSession()
{
	disconnectSession();
}

void SSHSession::disconnectSession()
{
	running.store(false, std::memory_order_relaxed);

	if(channel) {
		libssh2_channel_close(channel);
		libssh2_channel_free(channel);
		channel = nullptr;
	}

	if (session) {
		libssh2_session_disconnect(session, "Normal Shutdown");
		libssh2_session_free(session);
		session = nullptr;
	}

#ifdef _WIN32
	if (sock != INVALID_SOCKET) { closesocket(sock); sock = INVALID_SOCKET; }
	WSACleanup();
#else
	if (sock >= 0) { ::close(sock); sock = -1; }
#endif
	
	emit disconnected();
}

void SSHSession::writeData(const QByteArray& data)
{
	if (!channel)
		return;

	libssh2_channel_write(channel, data.data(), data.size());
}

void SSHSession::run()
{
	running.store(true, std::memory_order_relaxed);

#ifdef _WIN32
	WSADATA wsadata;
	int err = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (err != 0) {
		emit connectionFailed("WSAStartup failed");
		return;
	}
#endif

	// --- SOCKET ---
	sock = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
	if (sock == INVALID_SOCKET) {
#else
	if (sock < 0) {
#endif
		emit connectionFailed("Impossible de créer le socket");
		return;
	}

	sockaddr_in sin{};
	sin.sin_family = AF_INET;
	sin.sin_port = htons(static_cast<u_short>(port));

#ifdef _WIN32
	if (InetPtonA(AF_INET, host.toUtf8().constData(), &sin.sin_addr) != 1) {
		emit connectionFailed("Adresse IP invalide : " + host);
		return;
	}
#else
	if (inet_pton(AF_INET, host.toUtf8().constData(), &sin.sin_addr) != 1) {
		emit connectionFailed("Adresse IP invalide : " + host);
		return;
	}
#endif

	if (::connect(sock, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) != 0) {
		emit connectionFailed("Impossible de se connecter au serveur");
		return;
	}

	// --- Session SSH ---
	session = libssh2_session_init();
	if (!session) {
		emit connectionFailed("Impossible d'initialiser la session SSH");
		return;
	}

	if (libssh2_session_handshake(session, sock)) {
		emit connectionFailed("Handshake SSH échoué");
		return;
	}

	libssh2_session_set_blocking(session, 1);

	// --- Authentification ---
	int rc = LIBSSH2_ERROR_AUTHENTICATION_FAILED;

	if (authMethod == AuthMethod::Password) {

		rc = libssh2_userauth_password(
			session,
			username.toUtf8().constData(),
			password.toUtf8().constData()
		);
	}
	else {
		QByteArray userUtf8 = username.toUtf8();
		QByteArray passUtf8 = passphrase.toUtf8();

		rc = libssh2_userauth_publickey_fromfile(
			session,
			userUtf8.constData(),
			nullptr,	// clé publique → libssh2 la reconstruit
			privateKeyPath.toUtf8().constData(),   // clé privée
			passphrase.isEmpty() ? nullptr : passUtf8.constData()
		);
	}

	if (rc != 0) {
		char* errmsg = nullptr;
		libssh2_session_last_error(session, &errmsg, nullptr, 0);
		emit connectionFailed(QString("Authentification échouée : %1").arg(errmsg));
		return;
	}

	libssh2_session_set_blocking(session, 0);

	// --- Channel ---
	LIBSSH2_CHANNEL* ch = nullptr;
	do {
		ch = libssh2_channel_open_session(session);
		if (!ch) {
			int err = libssh2_session_last_errno(session);
			if (err == LIBSSH2_ERROR_EAGAIN) { msleep(10); continue; }
			char* errmsg = nullptr;
			libssh2_session_last_error(session, &errmsg, nullptr, 0);
			emit connectionFailed(QString("Impossible d'ouvrir un canal SSH : %1").arg(errmsg));
			return;
		}
	} while (!ch);
	channel = ch;

	// --- PTY ---
	do {
		rc = libssh2_channel_request_pty(channel, "xterm-256color");
		if (rc == LIBSSH2_ERROR_EAGAIN) msleep(10);
	} while (rc == LIBSSH2_ERROR_EAGAIN);
	if (rc != 0) {
		char* errmsg = nullptr;
		libssh2_session_last_error(session, &errmsg, nullptr, 0);
		emit connectionFailed(QString("Impossible de demander un PTY : %1").arg(errmsg));
		return;
	}

	// --- Shell ---
	do {
		rc = libssh2_channel_shell(channel);
		if (rc == LIBSSH2_ERROR_EAGAIN) msleep(10);
	} while (rc == LIBSSH2_ERROR_EAGAIN);
	if (rc != 0) {
		char* errmsg = nullptr;
		libssh2_session_last_error(session, &errmsg, nullptr, 0);
		emit connectionFailed(QString("Impossible de lancer le shell : %1").arg(errmsg));
		return;
	}

	emit connected();

	// --- Boucle de lecture ---
	char bufferOut[4096];
	char bufferErr[4096];

	while (running) {
		long n = libssh2_channel_read(channel, bufferOut, sizeof(bufferOut));
		long n2 = libssh2_channel_read_stderr(channel, bufferErr, sizeof(bufferErr));

		if (n == LIBSSH2_ERROR_EAGAIN && n2 == LIBSSH2_ERROR_EAGAIN) {
			msleep(10);
			continue;
		}

		if (n > 0)  emit dataReceived(QByteArray(bufferOut, static_cast<int>(n)));
		if (n2 > 0) emit dataReceived(QByteArray(bufferErr, static_cast<int>(n2)));

		if (libssh2_channel_eof(channel)) break;
	}

	disconnectSession();
}

void SSHSession::resizePty(int cols, int rows)
{
	if (!channel) return;
	libssh2_channel_request_pty_size(channel, cols, rows);
}