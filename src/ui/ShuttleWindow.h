#pragma once
#include <QMainWindow>
#include <QTabWidget>

class ShuttleWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit ShuttleWindow(QWidget* parent = nullptr);

signals:
	void requestConnect(const QString& tunnelName);
	void requestDisconnect(const QString& tunnelName); //Systray on s'en occupe après

private:
	QTabWidget* tabs;
	QDockWidget* profileDock;
};