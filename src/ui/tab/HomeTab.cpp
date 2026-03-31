#include "HomeTab.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

HomeTab::HomeTab(QWidget* parent)
	: QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	QLabel* welcomeLabel = new QLabel("Bienvenue dans Shuttle !", this);
	welcomeLabel->setAlignment(Qt::AlignCenter);
	layout->addWidget(welcomeLabel);
	newSessionButton = new QPushButton("Nouvelle session", this);
	connect(newSessionButton, &QPushButton::clicked, this, &HomeTab::newSessionRequested);
	layout->addWidget(newSessionButton);
	layout->addStretch();
}

void HomeTab::onNewSessionClicked()
{
	emit newSessionRequested();
}