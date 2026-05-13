#include "HomeTab.h"
#include "../../core/TranslationManager.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

HomeTab::HomeTab(QWidget* parent)
	: QWidget(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
    connect(&TranslationManager::instance(), &TranslationManager::languageChanged,
        this, &HomeTab::retranslate);
	welcomeLabel = new QLabel(tr("Bienvenue dans Shuttle !"), this);
	welcomeLabel->setAlignment(Qt::AlignCenter);
	layout->addWidget(welcomeLabel);
	newSessionButton = new QPushButton(tr("Nouvelle session"), this);
	connect(newSessionButton, &QPushButton::clicked, this, &HomeTab::newSessionRequested);
	layout->addWidget(newSessionButton);
	layout->addStretch();
}

void HomeTab::onNewSessionClicked()
{
	emit newSessionRequested();
}

void HomeTab::retranslate()
{
	welcomeLabel->setText(tr("Bienvenue dans Shuttle !"));
	newSessionButton->setText(tr("Nouvelle session"));
}