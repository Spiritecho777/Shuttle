#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

class ShuttleWindow;

class TrayManager : public QObject
{
	Q_OBJECT

public:
	explicit TrayManager(ShuttleWindow* main, QObject* parent = nullptr);

private:
	QSystemTrayIcon* tray;
	QMenu* trayMenu;
};