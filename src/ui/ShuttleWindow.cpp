#include "ShuttleWindow.h"
#include "tab/HomeTab.h"
#include "ProfileListWidget.h"

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
	resizeDocks({ profileDock }, { 200 }, { Qt::Horizontal });

	connect(profileList, &ProfileListWidget::profileSelected, this, &ShuttleWindow::openSession);

    // --- Barre de menus ---
    //QMenu* sessionMenu = menuBar()->addMenu("Sessions");
    //sessionMenu->addAction("Nouvelle session", this, &ShuttleWindow::openSession);
    //sessionMenu->addAction("Fermer la session", this, &ShuttleWindow::closeCurrentSession);

    // --- Barre d’état ---
    statusBar()->showMessage("Prêt");
}

void ShuttleWindow::openSession(const SessionProfile& profile)
{

}

void ShuttleWindow::openNewProfileDialog()
{
    NewSessionDialog* dialog = new NewSessionDialog(this);
    connect(dialog, &NewSessionDialog::profileCreated, this, &ShuttleWindow::onProfileCreated);
    dialog->exec();
}

void ShuttleWindow::onProfileCreated(const QVariant& var)
{
    SessionProfile profile = var.value<SessionProfile>();
	profileStore->addProfile(profile);
}
