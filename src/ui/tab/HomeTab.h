#pragma once

#include <QWidget>

class QPushButton;
class QListWidget;

class HomeTab : public QWidget
{
	Q_OBJECT

public:
	explicit HomeTab(QWidget* parent = nullptr);

signals:
	void newSessionRequested();

private slots:
	void onNewSessionClicked();

private:
	QPushButton* newSessionButton;
};