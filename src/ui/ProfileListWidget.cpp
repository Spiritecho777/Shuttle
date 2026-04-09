#include "ProfileListWidget.h"
#include "../ssh/ProfileStore.h"
#include "../ssh/SessionProfile.h"

#include <QVBoxLayout>

ProfileListWidget::ProfileListWidget(ProfileStore* store, QWidget* parent)
	: QWidget(parent), store(store)
{
	list = new QListWidget(this);
	
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(list);
	setLayout(layout);
	
	connect(store, &ProfileStore::profilesChanged, this, &ProfileListWidget::refreshList);
	connect(list, &QListWidget::itemClicked, this, &ProfileListWidget::onItemClicked);
	
	refreshList();
}

void ProfileListWidget::refreshList()
{
	list->clear();

	for (const SessionProfile& profile : store->profiles()) {
		QListWidgetItem* item = new QListWidgetItem(profile.name);
		item->setData(Qt::UserRole, profile.host);
		list->addItem(item);
	}
}

void ProfileListWidget::onItemClicked(QListWidgetItem* item)
{
	QString name = item->text();
	
	for (const SessionProfile& profile : store->profiles()) {
		if (profile.name == name) {
			emit profileSelected(profile);
			return;
		}
	}
}