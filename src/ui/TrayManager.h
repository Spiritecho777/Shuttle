#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

class ShuttleWindow;

enum class TunnelState {
	Disconnected,
	Connected,
	Error
};

class TrayManager : public QObject
{
	Q_OBJECT

public:
	explicit TrayManager(ShuttleWindow* main, QObject* parent = nullptr);

	void refreshTunnelMenus(const QList<QPair<QString, bool>>& tunnels);
	void retranslate();

public slots:
	void updateIcon(TunnelState state);

private:
	QSystemTrayIcon* tray;
	QMenu* trayMenu;
	QMenu* menuConnect;
	QMenu* menuDisconnect;
	QAction* actionShow;
	QAction* actionQuit;

	ShuttleWindow* mainWindow;
};