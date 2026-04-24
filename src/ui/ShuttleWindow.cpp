#include "ShuttleWindow.h"
#include "tab/HomeTab.h"
#include "ProfileListWidget.h"
#include "../ssh/SessionProfile.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabBar>
#include <QDebug>

ShuttleWindow::ShuttleWindow(QWidget* parent)
    : QMainWindow(parent)
{
	this->setWindowTitle("Shuttle");
    this->resize(1020, 700);
    // --- Zone centrale ---
    tabs = new QTabWidget(this);
    tabs->setTabsClosable(true);
    setCentralWidget(tabs);

    homeTab = new HomeTab();
	profileStore = new ProfileStore(this);
	tabs->addTab(homeTab, "Accueil");
	tabs->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr); // Empêche la fermeture de l'onglet d'accueil
	tabs->tabBar()->setTabButton(0, QTabBar::LeftSide, nullptr); // Empêche la fermeture de l'onglet d'accueil

    connect(homeTab, &HomeTab::newSessionRequested, this, &ShuttleWindow::openNewProfileDialog);

    // --- Dock latéral : profils SSH ---
    profileDock = new QDockWidget("Profils SSH", this);
    profileDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    profileList = new ProfileListWidget(profileStore, this);
    profileDock->setWidget(profileList);
    addDockWidget(Qt::LeftDockWidgetArea, profileDock);
	resizeDocks({ profileDock }, { 150 }, { Qt::Horizontal });

	m_sftpWidget = new SftpWidget(this);
	sftpDock = new QDockWidget("SFTP", this);
	sftpDock->setWidget(m_sftpWidget);
	sftpDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, sftpDock);
    resizeDocks({ sftpDock }, { 300 }, { Qt::Horizontal });
	sftpDock->hide(); // Caché par défaut, s'affiche à la connexion

	connect(profileList, &ProfileListWidget::profileSelected, this, &ShuttleWindow::openSession);
	connect(profileList, &ProfileListWidget::profileDeletedRequested, this, &ShuttleWindow::deleteSession);
	connect(profileList, &ProfileListWidget::profileEditRequested, this, &ShuttleWindow::editSession);

	connect(m_sftpWidget, &SftpWidget::statusMessage, this, [this](const QString& msg) {
        statusBar()->showMessage(msg);
		});

	connect(tabs, &QTabWidget::tabCloseRequested, this, &ShuttleWindow::closeTab);
    connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
        // Ignore si on est en train de construire un onglet
        QWidget* w = tabs->widget(index);
        if (!w || w == homeTab) return;

        auto* terminal = qobject_cast<TerminalWidget*>(w);
        if (!terminal) return;

        const SessionProfile& p = terminal->profile();
        if (p.host.isEmpty() || p.username.isEmpty()) return;  // ← ajoute username

        if (m_sftpWidget->isConnected())
            m_sftpWidget->disconnectSession();

        m_sftpWidget->connectTo(p);
        sftpDock->show();
    });

    // --- Barre de menus ---
    //QMenu* sessionMenu = menuBar()->addMenu("Sessions");
    //sessionMenu->addAction("Nouvelle session", this, &ShuttleWindow::openSession);
    //sessionMenu->addAction("Fermer la session", this, &ShuttleWindow::closeCurrentSession);

    // --- Barre d’état ---
    statusBar()->showMessage("Prêt");
}

void ShuttleWindow::openSession(const SessionProfile& profile)
{
    // --- Création session SSH ---
    auto* session = new SSHSession(
        profile.host,
        profile.port,
        profile.username,
        profile.password,
        profile.privateKeyPath,
        profile.passphrase,
        this
    );
    session->setAuthMethod(profile.authMethod);

    // --- Terminal ---
    auto* terminal = new TerminalWidget(this);

    // --- Titre onglet dynamique (OSC) ---
    connect(terminal, &TerminalWidget::titleChanged, this, [this, terminal](const QString& title) {
        int i = tabs->indexOf(terminal);
        if (i >= 0) tabs->setTabText(i, title);
        });

    // --- Fermeture onglet ---
    connect(tabs, &QTabWidget::tabCloseRequested, this, [this, terminal, session](int index) {
        if (tabs->widget(index) != terminal) return;
        session->disconnectSession();
        session->wait(2000);
        session->deleteLater();
        tabs->removeTab(index);
        terminal->deleteLater();
        });

    // --- Session fermée côté serveur ---
    connect(terminal, &TerminalWidget::sessionClosed, this, [this, terminal]() {
        int i = tabs->indexOf(terminal);
        if (i >= 0) tabs->setTabText(i, tabs->tabText(i) + " [fermé]");
        });

    // --- Statut ---
    connect(session, &SSHSession::connected, this, [this, profile]() {
        statusBar()->showMessage("Connecté : " + profile.name);
        });
    connect(session, &SSHSession::connectionFailed, this, [this](const QString& err) {
        statusBar()->showMessage("Erreur : " + err);
        });

    // --- Attache et lance ---
    terminal->attachSession(session);
	terminal->setProfile(profile); // pour SFTP
    session->start();

    // --- Onglet ---
    int idx = tabs->addTab(terminal, profile.name);
    tabs->setCurrentIndex(idx);

	sftpDock->show();
	m_sftpWidget->connectTo(profile);
}

void ShuttleWindow::deleteSession(int index)
{
	profileStore->removeProfile(index);
}

void ShuttleWindow::editSession(const SessionProfile& profile, int index)
{
	NewSessionDialog* dialog = new NewSessionDialog(this);
	dialog->loadProfile(profile, index);
	connect(dialog, &NewSessionDialog::profileEdited, this, &ShuttleWindow::onProfileEdited);
	dialog->exec();
}

void ShuttleWindow::onProfileEdited(const SessionProfile& profile, int index)
{
    profileStore->updateProfile(index, profile);
}


void ShuttleWindow::openNewProfileDialog()
{
    NewSessionDialog* dialog = new NewSessionDialog(this);
    connect(dialog, &NewSessionDialog::profileCreated, this, &ShuttleWindow::onProfileCreated);
    dialog->exec();
}

void ShuttleWindow::onProfileCreated(const SessionProfile& profile)
{
	profileStore->addProfile(profile);
}

void ShuttleWindow::closeTab(int index)
{
    QWidget* w = tabs->widget(index);
    if (!w || w == homeTab) return;

    auto* terminal = qobject_cast<TerminalWidget*>(w);
    if (terminal) {
        SSHSession* s = terminal->currentSession();
        terminal->detachSession();
        if (s) {
            s->disconnectSession();
            s->wait(2000);
            s->deleteLater();
        }
    }

    tabs->removeTab(index);

    if (tabs->count() <= 2) {
		sftpDock->hide();
		m_sftpWidget->disconnectSession();
    }

    w->deleteLater();
}
