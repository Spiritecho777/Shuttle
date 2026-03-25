#include "TrayManager.h"
#include "ShuttleWindow.h"

#include <QAction>
#include <QApplication>

TrayManager::TrayManager(ShuttleWindow* main, QObject* parent) : QObject(parent)
{
	tray = new QSystemTrayIcon(QIcon(":/Asset/Icone.png"), this);
	trayMenu = new QMenu();

	QAction* actionShow = trayMenu->addAction("Ouvrir");
	QAction* actionQuit = trayMenu->addAction("Quitter");

    tray->setContextMenu(trayMenu);

    connect(actionShow, &QAction::triggered, [main]() {
        main->show();
        main->raise();
        main->activateWindow();
    });

    connect(actionQuit, &QAction::triggered, qApp, &QApplication::quit);

    QObject::connect(tray, &QSystemTrayIcon::activated, [main](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            main->show();
            main->raise();
            main->activateWindow();
        }
    });

    tray->show();
}