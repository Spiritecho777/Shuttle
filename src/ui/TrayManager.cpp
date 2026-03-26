#include "TrayManager.h"
#include "ShuttleWindow.h"

#include <QAction>
#include <QApplication>

TrayManager::TrayManager(ShuttleWindow* main, QObject* parent) : QObject(parent)
{
	tray = new QSystemTrayIcon(QIcon("/icons/Asset/IconeOFF.png"), this);
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

void TrayManager::updateIcon(TunnelState state)
{
    switch (state)
    {
    case TunnelState::Disconnected:
        tray->setIcon(QIcon("/icons/Asset/IconeOFF.png"));
        break;

    case TunnelState::Connected:
        tray->setIcon(QIcon("/icons/Asset/IconeON.png"));
        break;

    case TunnelState::Error:
        tray->setIcon(QIcon("/icons/Asset/Icone.png"));
        break;
    }
}
