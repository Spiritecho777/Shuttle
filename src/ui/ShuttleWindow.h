#pragma once

#include "../ssh/SessionProfile.h"
#include "../ssh/ProfileStore.h"
#include "../ssh/SSHSession.h"
#include "../terminal/TerminalWidget.h"
#include "../sftp/SftpWidget.h"
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
	void onProfileCreated(const SessionProfile& profile);
	void onProfileEdited(const SessionProfile& profile, int index);
	void deleteSession(int index);
	void editSession(const SessionProfile& profile, int index);
	void closeTab(int index);

private:
	QTabWidget* tabs;
	QDockWidget* profileDock;
	QDockWidget* sftpDock;

	HomeTab* homeTab;
	ProfileStore* profileStore;
	ProfileListWidget* profileList;
	SftpWidget* m_sftpWidget = nullptr;
};