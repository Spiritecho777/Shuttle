#pragma once

#include <QWidget>
#include <QLabel>

class QPushButton;
class QListWidget;

class HomeTab : public QWidget
{
	Q_OBJECT

public:
	explicit HomeTab(QWidget* parent = nullptr);

	void retranslate();

signals:
	void newSessionRequested();

private slots:
	void onNewSessionClicked();

private:
	QPushButton* newSessionButton;
	QLabel* welcomeLabel;
};