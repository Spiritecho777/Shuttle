#include "TrayManager.h"
#include "ShuttleWindow.h"
#include "../core/TranslationManager.h"

#include <QAction>
#include <QApplication>

TrayManager::TrayManager(ShuttleWindow* main, QObject* parent) : QObject(parent), mainWindow(main)
{
	tray = new QSystemTrayIcon(QIcon(":/icons/Asset/IconeOFF.png"), this);
	trayMenu = new QMenu();

    menuConnect = trayMenu->addMenu(tr("Connexion"));
    menuDisconnect = trayMenu->addMenu(tr("Déconnexion"));
    
    trayMenu->addSeparator();

	actionShow = trayMenu->addAction(tr("Ouvrir"));
	actionQuit = trayMenu->addAction(tr("Quitter"));

    tray->setContextMenu(trayMenu);

    connect(actionShow, &QAction::triggered, [main]() {
        main->show();
        main->raise();
        main->activateWindow();
    });

    connect(actionQuit, &QAction::triggered, [main]() { main->close();  qApp->quit(); });

    QObject::connect(tray, &QSystemTrayIcon::activated, [main](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            main->show();
            main->raise();
            main->activateWindow();
        }
    });

    tray->show();

    connect(&TranslationManager::instance(), &TranslationManager::languageChanged,
        this, &TrayManager::retranslate);
}

void TrayManager::refreshTunnelMenus(const QList<QPair<QString, bool>>& tunnels)
{
    menuConnect->clear();
    menuDisconnect->clear();

    bool anyConnected = false;

    for (const auto& pair : tunnels)
    {
        const QString& name = pair.first;
        bool connected = pair.second;

        if (connected)
        {
            anyConnected = true;
            QAction* action = menuDisconnect->addAction(name);
            connect(action, &QAction::triggered, this, [this, name]() {
                emit mainWindow->requestDisconnect(name);
                });
        }
        else
        {
            QAction* action = menuConnect->addAction(name);
            connect(action, &QAction::triggered, this, [this, name]() {
                emit mainWindow->requestConnect(name);
                });
        }
    }

    // Désactiver les menus vides
    menuConnect->setEnabled(!menuConnect->isEmpty());
    menuDisconnect->setEnabled(!menuDisconnect->isEmpty());

    // Icône globale
    updateIcon(anyConnected ? TunnelState::Connected : TunnelState::Disconnected);
}

void TrayManager::updateIcon(TunnelState state)
{
    switch (state)
    {
    case TunnelState::Disconnected:
        tray->setIcon(QIcon(":/icons/Asset/IconeOFF.png"));
        break;

    case TunnelState::Connected:
        tray->setIcon(QIcon(":/icons/Asset/IconeON.png"));
        break;

    case TunnelState::Error:
        tray->setIcon(QIcon(":/icons/Asset/Icone.png"));
        break;
    }
}

void TrayManager::retranslate()
{
    menuConnect->setTitle(tr("Connexion"));
    menuDisconnect->setTitle(tr("Déconnexion"));
	actionShow->setText(tr("Ouvrir"));
    actionQuit->setText(tr("Quitter"));
}
