#pragma once

#include "../ssh/SessionProfile.h"
#include "../ssh/ProfileStore.h"
#include "NewSessionDialog.h"

#include <QMainWindow>
#include <QTabWidget>
#include <QDockWidget>

class HomeTab;
class ProfileStore;
class ProfileListWidget;

class ShuttleWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit ShuttleWindow(QWidget* parent = nullptr);

	void openSession(const SessionProfile& profile);

signals:
	void requestConnect(const QString& tunnelName);
	void requestDisconnect(const QString& tunnelName); //Systray on s'en occupe après

private slots:
	void openNewProfileDialog();
	void onProfileCreated(const QVariant& profile);

private:
	QTabWidget* tabs;
	QDockWidget* profileDock;
	QDockWidget* sftpDock;

	HomeTab* homeTab;
	ProfileStore* profileStore;
	ProfileListWidget* profileList;
};