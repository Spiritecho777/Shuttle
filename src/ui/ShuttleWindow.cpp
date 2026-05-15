#include "ShuttleWindow.h"
#include "tab/HomeTab.h"
#include "ProfileListWidget.h"
#include "../ssh/SessionProfile.h"
#include "../core/TranslationManager.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabBar>
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QActionGroup>

ShuttleWindow::ShuttleWindow(QWidget* parent)
    : QMainWindow(parent)
{
	this->setWindowTitle("Shuttle");
    this->resize(1020, 700);

    // --- Menu ---
    langMenu = menuBar()->addMenu(tr("Langue"));

    QActionGroup* langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);

    QAction* actFr = langMenu->addAction("Français");
    QAction* actEn = langMenu->addAction("English");
    QAction* actBz = langMenu->addAction("Brezhoneg");
    QAction* actJa = langMenu->addAction("日本語");

    actFr->setCheckable(true);
    actEn->setCheckable(true);
	actBz->setCheckable(true);
	actJa->setCheckable(true);

    langGroup->addAction(actFr);
    langGroup->addAction(actEn);
	langGroup->addAction(actBz);
	langGroup->addAction(actJa);

    connect(actFr, &QAction::triggered, this, [this]() { setLanguage("fr"); });
    connect(actEn, &QAction::triggered, this, [this]() { setLanguage("en"); });
    connect(actBz, &QAction::triggered, this, [this]() { setLanguage("bz"); });
    connect(actJa, &QAction::triggered, this, [this]() { setLanguage("ja"); });

    connect(&TranslationManager::instance(), &TranslationManager::languageChanged,
        this, &ShuttleWindow::retranslateUi);

    // --- Zone centrale ---
    tabs = new QTabWidget(this);
    tabs->setTabsClosable(true);
    setCentralWidget(tabs);

    homeTab = new HomeTab();
	profileStore = new ProfileStore(this);
	tabs->addTab(homeTab, tr("Accueil"));
	tabs->tabBar()->setTabButton(0, QTabBar::RightSide, nullptr); // Empêche la fermeture de l'onglet d'accueil
	tabs->tabBar()->setTabButton(0, QTabBar::LeftSide, nullptr); // Empêche la fermeture de l'onglet d'accueil

    connect(homeTab, &HomeTab::newSessionRequested, this, &ShuttleWindow::openNewProfileDialog);

    // --- Dock latéral : profils SSH ---
    profileDock = new QDockWidget(tr("Profils SSH"), this);
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
    connect(profileList, &ProfileListWidget::tunnelStartRequested, this, &ShuttleWindow::startTunnel);
    connect(profileList, &ProfileListWidget::tunnelStopRequested, this, &ShuttleWindow::stopTunnel);

	connect(m_sftpWidget, &SftpWidget::statusMessage, this, [this](const QString& msg) {
        statusBar()->showMessage(msg);
		});

	connect(tabs, &QTabWidget::tabCloseRequested, this, &ShuttleWindow::closeTab);
    connect(tabs, &QTabWidget::currentChanged, this, [this](int index) {
        QWidget* w = tabs->widget(index);
        if (!w || w == homeTab) return;
        auto* terminal = qobject_cast<TerminalWidget*>(w);
        if (!terminal) return;
        const SessionProfile& p = terminal->profile();
        if (p.host.isEmpty() || p.username.isEmpty()) return;
        if (m_sftpWidget->isConnected() && m_currentHost == p.host) return;

        m_currentHost = p.host;
        if (m_sftpWidget->isConnected()) m_sftpWidget->disconnectSession();
        m_sftpWidget->connectTo(p);
        m_monitorBar->connectTo(p);
        sftpDock->show();
        });

    // --- Barre d’état ---
	m_monitorBar = new MonitorBar(this);
	statusBar()->addPermanentWidget(m_monitorBar, 1);
	statusBar()->setSizeGripEnabled(false);

	m_tray = new TrayManager(this, this);
    connect(this, &ShuttleWindow::requestConnect, this, [this](const QString& name) {
        for (const SessionProfile& p : profileStore->profiles())
            if (p.name == name) { startTunnel(p); return; }
        });

    connect(this, &ShuttleWindow::requestDisconnect, this, [this](const QString& name) {
		for (const SessionProfile& p : profileStore->profiles())
            if (p.name == name) { stopTunnel(p); return; }
        });

	refreshTray();
}

// Profil
void ShuttleWindow::openSession(const SessionProfile& profile)
{
    // --- Création session SSH ---
    auto* session = new SSHSession(
        profile.host,
        profile.port,
        profile.username,
        profile.password,
        profile.privateKeyPath,
		profile.portTunnel,
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

    // --- Session fermée côté serveur ---
    connect(terminal, &TerminalWidget::sessionClosed, this, [this, terminal]() {
        int i = tabs->indexOf(terminal);
        if (i >= 0) tabs->setTabText(i, tabs->tabText(i) + tr(" [fermé]"));

		//Déconnecte le SFTP si c'était la session active
        if (m_sftpWidget->isConnected()) {
            const SessionProfile& p = terminal->profile();
            if (m_sftpWidget->isConnected())
                m_sftpWidget->disconnectSession();
        }
    });

    // --- Statut ---
    connect(session, &SSHSession::connected, this, [this, profile]() {
        statusBar()->showMessage(tr("Connecté : ") + profile.name);
        });
    connect(session, &SSHSession::connectionFailed, this, [this](const QString& err) {
        statusBar()->showMessage(tr("Erreur : ") + err);
        });

    // --- Attache et lance ---
    terminal->attachSession(session);
	terminal->setProfile(profile); // pour SFTP
    session->start();

    // --- Onglet ---
    int idx = tabs->addTab(terminal, profile.name);
    tabs->setCurrentIndex(idx);

	sftpDock->show();
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

// Tab
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
            s->wait(5000);
            s->deleteLater();
        }
    }

    tabs->removeTab(index);

    if (tabs->count() <= 1) {
		sftpDock->hide();
		m_sftpWidget->disconnectSession();
		m_monitorBar->disconnectSession();
        m_currentHost.clear();
    }

    w->deleteLater();
}

// Tunnel
bool ShuttleWindow::isTunnelConnected(const QString& tunnelName) const
{
	return m_tunnels.contains(tunnelName);
}

void ShuttleWindow::startTunnel(const SessionProfile& profile)
{
	if (profile.portTunnel <= 0) return;

    if (m_tunnels.contains(profile.name)) return;

	auto* proc = new QProcess(this);

#ifndef _WIN32
    // Corrige les permissions de la clé si nécessaire
    QFile keyFile(profile.privateKeyPath);
    keyFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif

    QString cmd = "ssh";
	QStringList args;

    //args << "-f" << "-N";
    args << "-N";
	args << "-i" << profile.privateKeyPath;
    args << "-o" << "BatchMode=yes";
	args << "-o" << "StrictHostKeyChecking=no";
	args << "-o" << "PasswordAuthentication=no";
	args << "-o" << "UserKnownHostsFile=/dev/null";
	args << "-o" << "ExitOnForwardFailure=yes";
	args << "-o" << "ServerAliveInterval=30";
	args << "-o" << "ServerAliveCountMax=3";
	args << "-p" << QString::number(profile.port);
    args << "-L" << QString("%1:127.0.0.1:%1").arg(profile.portTunnel);
	args << QString("%1@%2").arg(profile.username).arg(profile.host);

#ifndef _WIN32
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert("SSH_ASKPASS_REQUIRE", "never");
	proc->setProcessEnvironment(env);
#endif

    connect(proc, &QProcess::readyReadStandardError, this, [proc]() {
        qDebug() << "SSH tunnel stderr:" << proc->readAllStandardError();
        });

    connect(proc, &QProcess::started, this, [this, profile, proc]() {
        m_tunnels[profile.name] = proc;
        statusBar()->showMessage(tr("Tunnel démarré : ") + profile.name);
        refreshTray();
        });

    connect(proc, &QProcess::finished, this, [this, profile, proc](int exitCode) {
        m_tunnels.remove(profile.name);
        statusBar()->showMessage(exitCode == 0
            ? tr("Tunnel fermé : ") + profile.name
            : tr("Tunnel perdu : ") + profile.name);
        proc->deleteLater();
        refreshTray();
        });

    proc->start(cmd, args);

    if (!proc->waitForStarted(2000)) {
        proc->deleteLater();
        statusBar()->showMessage(tr("Échec du tunnel : ") + profile.name);
        refreshTray();
    }
}

void ShuttleWindow::stopTunnel(const SessionProfile& profile)
{
	if (!m_tunnels.contains(profile.name)) return;

	QProcess* proc = m_tunnels[profile.name];
	proc->kill();
	proc->waitForFinished(1000);
	proc->deleteLater();

	m_tunnels.remove(profile.name);
	statusBar()->showMessage(tr("Tunnel arrêté : ") + profile.name);
    refreshTray();
}

void ShuttleWindow::refreshTray()
{
	QList<QPair<QString, bool>> tunnels;
    for (const SessionProfile& p : profileStore->profiles()) {
        if (p.portTunnel > 0 && !p.privateKeyPath.isEmpty()) {
            tunnels.append({ p.name, m_tunnels.contains(p.name) });
        }
	}
	m_tray->refreshTunnelMenus(tunnels);
}

// Langue
void ShuttleWindow::setLanguage(const QString& lang)
{
    TranslationManager::instance().setLanguage(lang);
}

void ShuttleWindow::retranslateUi()
{
    profileDock->setWindowTitle(tr("Profils SSH"));
    tabs->setTabText(0, tr("Accueil"));

    langMenu->setTitle(tr("Langue"));

    homeTab->retranslate();
    m_tray->retranslate();
    m_monitorBar->retranslate();
}

