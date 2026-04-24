#include "SftpWidget.h"

#include <QHeaderView>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QApplication>
#include <QStyle>
#include <QFileInfo>

SftpWidget::SftpWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

SftpWidget::~SftpWidget()
{
    disconnectSession();
}

// -----------------------------------------------------------------
// UI
// -----------------------------------------------------------------

void SftpWidget::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // --- Toolbar ---
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(16, 16));

    m_actUp = m_toolbar->addAction(
        style()->standardIcon(QStyle::SP_ArrowUp), "Dossier parent");
    m_actRefresh = m_toolbar->addAction(
        style()->standardIcon(QStyle::SP_BrowserReload), "Actualiser");

    m_toolbar->addSeparator();

    m_actDownload = m_toolbar->addAction(
        style()->standardIcon(QStyle::SP_ArrowDown), "Télécharger");
    m_actUpload = m_toolbar->addAction(
        style()->standardIcon(QStyle::SP_ArrowUp), "Envoyer");

    m_toolbar->addSeparator();

    m_actNewFolder = m_toolbar->addAction(
        style()->standardIcon(QStyle::SP_FileDialogNewFolder), "Nouveau dossier");
    m_actRename = m_toolbar->addAction(
        style()->standardIcon(QStyle::SP_FileIcon), "Renommer");
    m_actDelete = m_toolbar->addAction(
        style()->standardIcon(QStyle::SP_TrashIcon), "Supprimer");

    layout->addWidget(m_toolbar);

    // --- Barre de chemin ---
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setPlaceholderText("Chemin distant...");
    m_pathEdit->setFont(QFont("Courier New", 9));
    layout->addWidget(m_pathEdit);

    // --- Vue fichiers ---
    m_model = new SftpModel(this);
    m_view = new QTreeView(this);
    m_view->setModel(m_model);
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);
    m_view->setSortingEnabled(true);
    m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_view->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_view->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_view->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    layout->addWidget(m_view, 1);

    // --- Barre de progression ---
    m_progress = new QProgressBar(this);
    m_progress->setVisible(false);
    m_progress->setMaximumHeight(4);
    m_progress->setTextVisible(false);
    layout->addWidget(m_progress);

    // --- Status ---
    m_statusLabel = new QLabel("Non connecté", this);
    m_statusLabel->setContentsMargins(4, 2, 4, 2);
    layout->addWidget(m_statusLabel);

    // Désactive tout jusqu'à connexion
    m_view->setEnabled(false);
    m_pathEdit->setEnabled(false);
    m_actUp->setEnabled(false);
    m_actRefresh->setEnabled(false);
    m_actDownload->setEnabled(false);
    m_actUpload->setEnabled(false);
    m_actNewFolder->setEnabled(false);
    m_actRename->setEnabled(false);
    m_actDelete->setEnabled(false);

    // --- Connexions UI ---
    connect(m_actUp, &QAction::triggered, this, &SftpWidget::navigateUp);
    connect(m_actRefresh, &QAction::triggered, this, &SftpWidget::refresh);
    connect(m_actDownload, &QAction::triggered, this, &SftpWidget::actionDownload);
    connect(m_actUpload, &QAction::triggered, this, &SftpWidget::actionUpload);
    connect(m_actNewFolder, &QAction::triggered, this, &SftpWidget::actionNewFolder);
    connect(m_actRename, &QAction::triggered, this, &SftpWidget::actionRename);
    connect(m_actDelete, &QAction::triggered, this, &SftpWidget::actionDelete);

    connect(m_pathEdit, &QLineEdit::returnPressed, this, &SftpWidget::onPathEntered);
    connect(m_view, &QTreeView::doubleClicked, this, &SftpWidget::onItemDoubleClicked);
    connect(m_view, &QTreeView::customContextMenuRequested, this, &SftpWidget::onContextMenu);
}

// -----------------------------------------------------------------
// Session
// -----------------------------------------------------------------

void SftpWidget::connectTo(const SessionProfile& profile)
{
    disconnectSession();

    m_statusLabel->setText("Connexion SFTP...");
    m_session = new SftpSession(profile, this);

    connect(m_session, &SftpSession::connected, this, &SftpWidget::onConnected);
    connect(m_session, &SftpSession::connectionFailed, this, &SftpWidget::onConnectionFailed);
    connect(m_session, &SftpSession::disconnected, this, &SftpWidget::onDisconnected);
    connect(m_session, &SftpSession::dirListed, this, &SftpWidget::onDirListed);
    connect(m_session, &SftpSession::operationError, this, &SftpWidget::onOperationError);
    connect(m_session, &SftpSession::operationSuccess, this, &SftpWidget::onOperationSuccess);
    connect(m_session, &SftpSession::downloadProgress, this, &SftpWidget::onDownloadProgress);
    connect(m_session, &SftpSession::uploadProgress, this, &SftpWidget::onUploadProgress);
    connect(m_session, &SftpSession::downloadFinished, this, &SftpWidget::onDownloadFinished);
    connect(m_session, &SftpSession::uploadFinished, this, &SftpWidget::onUploadFinished);
	connect(m_session, &SftpSession::homeResolved, this, &SftpWidget::navigateTo);

    m_session->start();
}

void SftpWidget::disconnectSession()
{
    if (!m_session) return;
    m_session->disconnectSession();
    m_session->wait(3000);
    m_session->deleteLater();
    m_session = nullptr;
    m_connected = false;
}

// -----------------------------------------------------------------
// Slots session
// -----------------------------------------------------------------

void SftpWidget::onConnected()
{
    m_connected = true;
    m_statusLabel->setText("Connecté");

    m_view->setEnabled(true);
    m_pathEdit->setEnabled(true);
    m_actUp->setEnabled(true);
    m_actRefresh->setEnabled(true);
    m_actDownload->setEnabled(true);
    m_actUpload->setEnabled(true);
    m_actNewFolder->setEnabled(true);
    m_actRename->setEnabled(true);
    m_actDelete->setEnabled(true);

    // Navigation vers /home/user ou / par défaut
    navigateTo("/");
}

void SftpWidget::onConnectionFailed(const QString& error)
{
    m_statusLabel->setText("Erreur : " + error);
    emit statusMessage("SFTP : " + error);
}

void SftpWidget::onDisconnected()
{
    m_connected = false;
    m_statusLabel->setText("Déconnecté");
    m_view->setEnabled(false);
    m_pathEdit->setEnabled(false);
}

void SftpWidget::onDirListed(const QString& path, const QList<SftpEntry>& entries)
{
    m_currentPath = path;
    m_pathEdit->setText(path);
    m_model->setEntries(path, entries);
    m_statusLabel->setText(QString("%1 éléments").arg(entries.size()));
}

void SftpWidget::onOperationError(const QString& msg)
{
    m_statusLabel->setText("Erreur : " + msg);
    emit statusMessage(msg);
}

void SftpWidget::onOperationSuccess(const QString& msg)
{
    m_statusLabel->setText(msg);
    refresh(); // Recharge le dossier courant
}

void SftpWidget::onDownloadProgress(const QString&, qint64 done, qint64 total)
{
    if (!m_progress->isVisible()) m_progress->setVisible(true);
    if (total > 0) {
        m_progress->setMaximum(static_cast<int>(total / 1024));
        m_progress->setValue(static_cast<int>(done / 1024));
    }
}

void SftpWidget::onUploadProgress(const QString&, qint64 done, qint64 total)
{
    onDownloadProgress({}, done, total);
}

void SftpWidget::onDownloadFinished(const QString& path)
{
    m_progress->setVisible(false);
    m_statusLabel->setText("Téléchargé : " + QFileInfo(path).fileName());
}

void SftpWidget::onUploadFinished(const QString& path)
{
    m_progress->setVisible(false);
    m_statusLabel->setText("Envoyé : " + QFileInfo(path).fileName());
    refresh();
}

// -----------------------------------------------------------------
// Navigation
// -----------------------------------------------------------------

void SftpWidget::navigateTo(const QString& path)
{
    if (!m_connected || !m_session) return;
    m_statusLabel->setText("Chargement...");
    m_session->listDir(path);
}

void SftpWidget::navigateUp()
{
    if (m_currentPath == "/" || m_currentPath.isEmpty()) return;

    QString parent = m_currentPath;
    if (parent.endsWith('/')) parent.chop(1);
    int idx = parent.lastIndexOf('/');
    parent = (idx <= 0) ? "/" : parent.left(idx);

    navigateTo(parent);
}

void SftpWidget::refresh()
{
    if (!m_currentPath.isEmpty())
        navigateTo(m_currentPath);
}

void SftpWidget::onPathEntered()
{
    navigateTo(m_pathEdit->text().trimmed());
}

void SftpWidget::onItemDoubleClicked(const QModelIndex& index)
{
    SftpEntry e = m_model->entry(index);
    if (e.isDir)
        navigateTo(e.fullPath);
    else
        actionDownload(); // double-clic sur fichier = télécharger
}

// -----------------------------------------------------------------
// Actions
// -----------------------------------------------------------------

SftpEntry SftpWidget::selectedEntry() const
{
    QModelIndex idx = m_view->currentIndex();
    return m_model->entry(idx);
}

QString SftpWidget::selectedPath() const
{
    return selectedEntry().fullPath;
}

void SftpWidget::actionDownload()
{
    SftpEntry e = selectedEntry();
    if (e.name.isEmpty() || e.isDir) return;

    QString localPath = QFileDialog::getSaveFileName(
        this, "Enregistrer sous", e.name);
    if (localPath.isEmpty()) return;

    m_statusLabel->setText("Téléchargement : " + e.name);
    m_session->downloadFile(e.fullPath, localPath);
}

void SftpWidget::actionUpload()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Choisir les fichiers à envoyer");
    if (files.isEmpty()) return;

    for (const QString& localPath : files) {
        QString remotePath = m_currentPath;
        if (!remotePath.endsWith('/')) remotePath += '/';
        remotePath += QFileInfo(localPath).fileName();

        m_statusLabel->setText("Envoi : " + QFileInfo(localPath).fileName());
        m_session->uploadFile(localPath, remotePath);
    }
}

void SftpWidget::actionRename()
{
    SftpEntry e = selectedEntry();
    if (e.name.isEmpty()) return;

    bool ok;
    QString newName = QInputDialog::getText(
        this, "Renommer", "Nouveau nom :", QLineEdit::Normal, e.name, &ok);
    if (!ok || newName.isEmpty() || newName == e.name) return;

    QString newPath = m_currentPath;
    if (!newPath.endsWith('/')) newPath += '/';
    newPath += newName;

    m_session->renameEntry(e.fullPath, newPath);
}

void SftpWidget::actionDelete()
{
    SftpEntry e = selectedEntry();
    if (e.name.isEmpty()) return;

    int ret = QMessageBox::question(
        this, "Supprimer",
        QString("Supprimer \"%1\" ?").arg(e.name),
        QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    m_session->deleteEntry(e.fullPath, e.isDir);
}

void SftpWidget::actionNewFolder()
{
    bool ok;
    QString name = QInputDialog::getText(
        this, "Nouveau dossier", "Nom du dossier :", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;

    QString path = m_currentPath;
    if (!path.endsWith('/')) path += '/';
    path += name;

    m_session->createDir(path);
}

// -----------------------------------------------------------------
// Menu contextuel
// -----------------------------------------------------------------

void SftpWidget::onContextMenu(const QPoint& pos)
{
    SftpEntry e = selectedEntry();
    QMenu menu(this);

    if (!e.name.isEmpty()) {
        if (!e.isDir)
            menu.addAction("Télécharger", this, &SftpWidget::actionDownload);
        menu.addAction("Renommer", this, &SftpWidget::actionRename);
        menu.addAction("Supprimer", this, &SftpWidget::actionDelete);
        menu.addSeparator();
    }

    menu.addAction("Envoyer un fichier", this, &SftpWidget::actionUpload);
    menu.addAction("Nouveau dossier", this, &SftpWidget::actionNewFolder);
    menu.addSeparator();
    menu.addAction("Actualiser", this, &SftpWidget::refresh);

    menu.exec(m_view->viewport()->mapToGlobal(pos));
}