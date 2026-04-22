#pragma once

#include <QThread>
#include <QString>
#include <QList>
#include <QDateTime>

#include <libssh2.h>
#include <libssh2_sftp.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "../ssh/AuthMethod.h"
#include "../ssh/SessionProfile.h"

struct SftpEntry {
    QString  name;
    QString  fullPath;
    bool     isDir = false;
    quint64  size = 0;
    QDateTime modified;
    uint     permissions = 0;
};

class SftpSession : public QThread
{
    Q_OBJECT

public:
    explicit SftpSession(const SessionProfile& profile, QObject* parent = nullptr);
    ~SftpSession();

    // Opérations — appelées depuis le thread UI, exécutées dans le thread SFTP
    // via le système de queue interne
    void listDir(const QString& path);
    void downloadFile(const QString& remotePath, const QString& localPath);
    void uploadFile(const QString& localPath, const QString& remotePath);
    void renameEntry(const QString& oldPath, const QString& newPath);
    void deleteEntry(const QString& path, bool isDir);
    void createDir(const QString& path);

    void disconnectSession();

signals:
    void connected();
    void connectionFailed(const QString& error);
    void disconnected();

    void dirListed(const QString& path, const QList<SftpEntry>& entries);
    void downloadProgress(const QString& path, qint64 done, qint64 total);
    void downloadFinished(const QString& path);
    void uploadProgress(const QString& path, qint64 done, qint64 total);
    void uploadFinished(const QString& path);
    void operationError(const QString& message);
    void operationSuccess(const QString& message);

protected:
    void run() override;

private:
    // Exécutées dans le thread SFTP
    void doListDir(const QString& path);
    void doDownload(const QString& remotePath, const QString& localPath);
    void doUpload(const QString& localPath, const QString& remotePath);
    void doRename(const QString& oldPath, const QString& newPath);
    void doDelete(const QString& path, bool isDir);
    void doCreateDir(const QString& path);

    bool initSocket();
    bool initSsh();
    bool initSftp();
    void cleanup();

    // Commande en attente — on utilise un système simple de lambda queue
    // protégé par mutex
    struct Command {
        enum Type { ListDir, Download, Upload, Rename, Delete, CreateDir, Quit };
        Type    type;
        QString arg1;
        QString arg2;
        bool    boolArg = false;
    };

    void enqueue(const Command& cmd);
    bool dequeue(Command& cmd);

    QMutex              m_mutex;
    QWaitCondition      m_cond;
    QList<Command>      m_queue;
    bool                m_running = false;

    // SSH
    SessionProfile      m_profile;

#ifdef _WIN32
    SOCKET              m_sock = INVALID_SOCKET;
#else
    int                 m_sock = -1;
#endif

    LIBSSH2_SESSION* m_session = nullptr;
    LIBSSH2_SFTP* m_sftp = nullptr;
};