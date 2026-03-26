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

public slots:
	void updateIcon(TunnelState state);

private:
	QSystemTrayIcon* tray;
	QMenu* trayMenu;
};