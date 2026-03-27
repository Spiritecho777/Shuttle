#pragma once
#include <QWidget>

class ShuttleWindow : public QWidget 
{
	Q_OBJECT

public:
	explicit ShuttleWindow(QWidget* parent = nullptr);

signals:
	void requestConnect(const QString& tunnelName);
	void requestDisconnect(const QString& tunnelName);
};