#include "SftpSession.h"

#include <QFile>
#include <QMutexLocker>
#include <QDebug>

#include <libssh2.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

SftpSession::SftpSession(const SessionProfile& profile, QObject* parent)
    : QThread(parent), m_profile(profile)
{}

SftpSession::~SftpSession()
{
    disconnectSession();
    wait(3000);
}

// -----------------------------------------------------------------
// API publique — enfile les commandes
// -----------------------------------------------------------------

void SftpSession::listDir(const QString& path)
{
    enqueue({ Command::ListDir, path, {}, false });
}

void SftpSession::downloadFile(const QString& remotePath, const QString& localPath)
{
    enqueue({ Command::Download, remotePath, localPath, false });
}

void SftpSession::uploadFile(const QString& localPath, const QString& remotePath)
{
    enqueue({ Command::Upload, localPath, remotePath, false });
}

void SftpSession::renameEntry(const QString& oldPath, const QString& newPath)
{
    enqueue({ Command::Rename, oldPath, newPath, false });
}

void SftpSession::deleteEntry(const QString& path, bool isDir)
{
    enqueue({ Command::Delete, path, {}, isDir });
}

void SftpSession::createDir(const QString& path)
{
    enqueue({ Command::CreateDir, path, {}, false });
}

void SftpSession::disconnectSession()
{
    enqueue({ Command::Quit, {}, {}, false });
}

// -----------------------------------------------------------------
// Queue thread-safe
// -----------------------------------------------------------------

void SftpSession::enqueue(const Command& cmd)
{
    QMutexLocker lock(&m_mutex);
    m_queue.append(cmd);
    m_cond.wakeOne();
}

bool SftpSession::dequeue(Command& cmd)
{
    QMutexLocker lock(&m_mutex);
    while (m_queue.isEmpty() && m_running)
        m_cond.wait(&m_mutex, 500);

    if (!m_running && m_queue.isEmpty())
        return false;

    if (!m_queue.isEmpty()) {
        cmd = m_queue.takeFirst();
        return true;
    }
    return false;
}

// -----------------------------------------------------------------
// Thread principal
// -----------------------------------------------------------------

void SftpSession::run()
{
    m_running = true;

    if (!initSocket() || !initSsh() || !initSftp()) {
        cleanup();
        return;
    }

    emit connected();

    // Boucle de traitement des commandes
    Command cmd;
    while (dequeue(cmd)) {
        switch (cmd.type) {
        case Command::ListDir:   doListDir(cmd.arg1);                        break;
        case Command::Download:  doDownload(cmd.arg1, cmd.arg2);             break;
        case Command::Upload:    doUpload(cmd.arg1, cmd.arg2);               break;
        case Command::Rename:    doRename(cmd.arg1, cmd.arg2);               break;
        case Command::Delete:    doDelete(cmd.arg1, cmd.boolArg);            break;
        case Command::CreateDir: doCreateDir(cmd.arg1);                      break;
        case Command::Quit:
            m_running = false;
            goto done;
        }
    }

done:
    cleanup();
    emit disconnected();
}

// -----------------------------------------------------------------
// Init connexion
// -----------------------------------------------------------------

bool SftpSession::initSocket()
{
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (m_sock == INVALID_SOCKET) {
#else
    if (m_sock < 0) {
#endif
        emit connectionFailed("SFTP : impossible de créer le socket");
        return false;
    }

    sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(static_cast<u_short>(m_profile.port));

#ifdef _WIN32
    if (InetPtonA(AF_INET, m_profile.host.toUtf8().constData(), &sin.sin_addr) != 1) {
#else
    if (inet_pton(AF_INET, m_profile.host.toUtf8().constData(), &sin.sin_addr) != 1) {
#endif
        emit connectionFailed("SFTP : adresse IP invalide");
        return false;
    }

    if (::connect(m_sock, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) != 0) {
        emit connectionFailed("SFTP : connexion refusée");
        return false;
    }

    return true;
    }

bool SftpSession::initSsh()
{
    m_session = libssh2_session_init();
    if (!m_session) {
        emit connectionFailed("SFTP : impossible d'initialiser la session SSH");
        return false;
    }

    libssh2_session_set_blocking(m_session, 1);

    if (libssh2_session_handshake(m_session, m_sock) != 0) {
        emit connectionFailed("SFTP : handshake SSH échoué");
        return false;
    }

    int rc = LIBSSH2_ERROR_AUTHENTICATION_FAILED;

    if (m_profile.authMethod == AuthMethod::Password) {
        rc = libssh2_userauth_password(
            m_session,
            m_profile.username.toUtf8().constData(),
            m_profile.password.toUtf8().constData()
        );
    }
    else {
        QByteArray passUtf8 = m_profile.passphrase.toUtf8();
        rc = libssh2_userauth_publickey_fromfile(
            m_session,
            m_profile.username.toUtf8().constData(),
            nullptr,
            m_profile.privateKeyPath.toUtf8().constData(),
            m_profile.passphrase.isEmpty() ? nullptr : passUtf8.constData()
        );
    }

    if (rc != 0) {
        char* errmsg = nullptr;
        libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        emit connectionFailed(QString("SFTP : auth échouée : %1").arg(errmsg));
        return false;
    }

    return true;
}

bool SftpSession::initSftp()
{
    m_sftp = libssh2_sftp_init(m_session);
    if (!m_sftp) {
        char* errmsg = nullptr;
        libssh2_session_last_error(m_session, &errmsg, nullptr, 0);
        emit connectionFailed(QString("SFTP : impossible d'initialiser SFTP : %1").arg(errmsg));
        return false;
    }

    char homeBuf[512];
    memset(homeBuf, 0, sizeof(homeBuf));  // ← zero out complet

    int rc = libssh2_sftp_realpath(m_sftp, ".", homeBuf, sizeof(homeBuf) - 1);
    if (rc > 0 && rc < static_cast<int>(sizeof(homeBuf))) {
        homeBuf[rc] = '\0';  // force null terminator à la bonne position
        QString home = QString::fromUtf8(homeBuf, rc);  // passe la longueur explicitement
        if (!home.isEmpty())
            emit homeResolved(home);
    }

    return true;
}

void SftpSession::cleanup()
{
    if (m_sftp) {
        libssh2_sftp_shutdown(m_sftp);
        m_sftp = nullptr;
    }
    if (m_session) {
        libssh2_session_disconnect(m_session, "Normal Shutdown");
        libssh2_session_free(m_session);
        m_session = nullptr;
    }
#ifdef _WIN32
    if (m_sock != INVALID_SOCKET) { closesocket(m_sock); m_sock = INVALID_SOCKET; }
#else
    if (m_sock >= 0) { ::close(m_sock); m_sock = -1; }
#endif
}

// -----------------------------------------------------------------
// Opérations SFTP
// -----------------------------------------------------------------

void SftpSession::doListDir(const QString & path)
{
    QByteArray pathUtf8 = path.toUtf8();

    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_opendir(m_sftp, pathUtf8.constData());
    if (!handle) {
        emit operationError(QString("Impossible d'ouvrir : %1").arg(path));
        return;
    }

    QList<SftpEntry> entries;
    std::vector<char> filename(4096, 0);
    std::vector<char> longentry(4096, 0);
    LIBSSH2_SFTP_ATTRIBUTES attrs{};

    while (libssh2_sftp_readdir_ex(handle, filename.data(), filename.size(), longentry.data(), longentry.size(), &attrs) > 0)
    {
        QString name = QString::fromUtf8(filename.data());
        if (name == "." || name == "..") continue;

        SftpEntry entry;
        entry.name = name;
        entry.fullPath = path.endsWith('/') ? path + name : path + '/' + name;
        entry.isDir = LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
        entry.size = (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) ? attrs.filesize : 0;
        entry.permissions = (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) ? attrs.permissions : 0;

        if (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME)
            entry.modified = QDateTime::fromSecsSinceEpoch(attrs.mtime);

        entries.append(entry);
    }

    libssh2_sftp_closedir(handle);

    // Trie : dossiers d'abord, puis fichiers, alphabétique
    std::sort(entries.begin(), entries.end(), [](const SftpEntry& a, const SftpEntry& b) {
        if (a.isDir != b.isDir) return a.isDir > b.isDir;
        return a.name.toLower() < b.name.toLower();
        });

    emit dirListed(path, entries);
}

void SftpSession::doDownload(const QString & remotePath, const QString & localPath)
{
    QByteArray remoteUtf8 = remotePath.toUtf8();

    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(
        m_sftp, remoteUtf8.constData(),
        LIBSSH2_FXF_READ, 0
    );

    if (!handle) {
        emit operationError(QString("Impossible d'ouvrir en lecture : %1").arg(remotePath));
        return;
    }

    // Taille du fichier
    LIBSSH2_SFTP_ATTRIBUTES attrs{};
    libssh2_sftp_fstat(handle, &attrs);
    qint64 total = (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) ? static_cast<qint64>(attrs.filesize) : -1;

    QFile localFile(localPath);
    if (!localFile.open(QIODevice::WriteOnly)) {
        libssh2_sftp_close(handle);
        emit operationError(QString("Impossible de créer le fichier local : %1").arg(localPath));
        return;
    }

    std::vector<char> buf(32768, 0);
    qint64 done = 0;
    long nread;

    while ((nread = libssh2_sftp_read(handle, buf.data(), buf.size())) > 0) {
        localFile.write(buf.data(), nread);
        done += nread;
        if (total > 0)
            emit downloadProgress(remotePath, done, total);
    }

    localFile.close();
    libssh2_sftp_close(handle);

    if (nread < 0) {
        emit operationError(QString("Erreur lecture SFTP : %1").arg(remotePath));
        return;
    }

    emit downloadFinished(remotePath);
}

void SftpSession::doUpload(const QString& localPath, const QString& remotePath)
{
    QFile localFile(localPath);
    if (!localFile.open(QIODevice::ReadOnly)) {
        emit operationError(QString("Impossible d'ouvrir le fichier local : %1").arg(localPath));
        return;
    }

    qint64 total = localFile.size();
    QByteArray remoteUtf8 = remotePath.toUtf8();

    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(
        m_sftp, remoteUtf8.constData(),
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
        LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH
    );

    if (!handle) {
        localFile.close();
        emit operationError(QString("Impossible de créer le fichier distant : %1").arg(remotePath));
        return;
    }

    std::vector<char> buf(32768, 0);
    qint64 done = 0;
    qint64 nread;

    while ((nread = localFile.read(buf.data(), static_cast<qint64>(buf.size()))) > 0) {
        char* ptr = buf.data();
        qint64 remaining = nread;
        while (remaining > 0) {
            long nwritten = libssh2_sftp_write(handle, ptr, static_cast<size_t>(remaining));
            if (nwritten < 0) {
                localFile.close();
                libssh2_sftp_close(handle);
                emit operationError(QString("Erreur écriture SFTP : %1").arg(remotePath));
                return;
            }
            ptr += nwritten;
            remaining -= nwritten;
            done += nwritten;
        }
        emit uploadProgress(remotePath, done, total);
    }

    localFile.close();
    libssh2_sftp_close(handle);
    emit uploadFinished(remotePath);
}

void SftpSession::doRename(const QString & oldPath, const QString & newPath)
{
    QByteArray oldUtf8 = oldPath.toUtf8();
    QByteArray newUtf8 = newPath.toUtf8();

    int rc = libssh2_sftp_rename_ex(m_sftp,
        oldUtf8.constData(), static_cast<unsigned int>(oldUtf8.size()),
        newUtf8.constData(), static_cast<unsigned int>(newUtf8.size()),
        LIBSSH2_SFTP_RENAME_OVERWRITE | LIBSSH2_SFTP_RENAME_ATOMIC
    );

    if (rc != 0)
        emit operationError(QString("Impossible de renommer : %1").arg(oldPath));
    else
        emit operationSuccess(QString("Renommé : %1 → %2").arg(oldPath, newPath));
}

void SftpSession::doDelete(const QString & path, bool isDir)
{
    QByteArray pathUtf8 = path.toUtf8();
    int rc;

    if (isDir)
        rc = libssh2_sftp_rmdir(m_sftp, pathUtf8.constData());
    else
        rc = libssh2_sftp_unlink(m_sftp, pathUtf8.constData());

    if (rc != 0)
        emit operationError(QString("Impossible de supprimer : %1").arg(path));
    else
        emit operationSuccess(QString("Supprimé : %1").arg(path));
}

void SftpSession::doCreateDir(const QString & path)
{
    QByteArray pathUtf8 = path.toUtf8();

    int rc = libssh2_sftp_mkdir(m_sftp, pathUtf8.constData(),
        LIBSSH2_SFTP_S_IRWXU |
        LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IXGRP |
        LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH
    );

    if (rc != 0)
        emit operationError(QString("Impossible de créer le dossier : %1").arg(path));
    else
        emit operationSuccess(QString("Dossier créé : %1").arg(path));
}