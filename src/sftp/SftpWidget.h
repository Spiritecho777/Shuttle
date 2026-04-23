#pragma once

#include <QWidget>
#include <QTreeView>
#include <QToolBar>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QAction>

#include "SftpSession.h"
#include "SftpModel.h"

class SftpWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SftpWidget(QWidget* parent = nullptr);
    ~SftpWidget();

    // Démarre une session SFTP avec ce profil
    void connectTo(const SessionProfile& profile);
    void disconnectSession();

    bool isConnected() const { return m_connected; }

signals:
    void statusMessage(const QString& msg);

private slots:
    void onConnected();
    void onConnectionFailed(const QString& error);
    void onDisconnected();
    void onDirListed(const QString& path, const QList<SftpEntry>& entries);
    void onOperationError(const QString& msg);
    void onOperationSuccess(const QString& msg);
    void onDownloadProgress(const QString& path, qint64 done, qint64 total);
    void onUploadProgress(const QString& path, qint64 done, qint64 total);
    void onDownloadFinished(const QString& path);
    void onUploadFinished(const QString& path);

    void onItemDoubleClicked(const QModelIndex& index);
    void onPathEntered();

    void navigateUp();
    void refresh();
    void actionDownload();
    void actionUpload();
    void actionRename();
    void actionDelete();
    void actionNewFolder();

    void onContextMenu(const QPoint& pos);

private:
    void setupUi();
    void navigateTo(const QString& path);
    QString selectedPath() const;
    SftpEntry selectedEntry() const;

    // UI
    QToolBar* m_toolbar = nullptr;
    QLineEdit* m_pathEdit = nullptr;
    QTreeView* m_view = nullptr;
    QProgressBar* m_progress = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Actions toolbar
    QAction* m_actUp = nullptr;
    QAction* m_actRefresh = nullptr;
    QAction* m_actDownload = nullptr;
    QAction* m_actUpload = nullptr;
    QAction* m_actNewFolder = nullptr;
    QAction* m_actRename = nullptr;
    QAction* m_actDelete = nullptr;

    // Data
    SftpSession* m_session = nullptr;
    SftpModel* m_model = nullptr;
    bool         m_connected = false;
    QString      m_currentPath;
    QList<QString> m_history; // navigation arrière
};