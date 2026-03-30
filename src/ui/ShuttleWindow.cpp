#include "ShuttleWindow.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QDebug>



ShuttleWindow::ShuttleWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // --- Zone centrale : les terminaux ---
    tabs = new QTabWidget(this);
    tabs->setTabsClosable(true);
    setCentralWidget(tabs);

    // --- Dock latéral : profils SSH ---
    profileDock = new QDockWidget("Profils SSH", this);
    profileDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    //profileDock->setWidget(new ProfileListWidget()); // ton widget
    addDockWidget(Qt::LeftDockWidgetArea, profileDock);

    // --- Barre de menus ---
    QMenu* sessionMenu = menuBar()->addMenu("Sessions");
    //sessionMenu->addAction("Nouvelle session", this, &ShuttleWindow::openNewSessionDialog);
    //sessionMenu->addAction("Fermer la session", this, &ShuttleWindow::closeCurrentSession);

    // --- Barre d’état ---
    statusBar()->showMessage("Prêt");
}
