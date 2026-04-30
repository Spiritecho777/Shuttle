#include "ProfileListWidget.h"
#include "../ssh/ProfileStore.h"
#include "../ssh/SessionProfile.h"

#include <QVBoxLayout>
#include <QMenu>

ProfileListWidget::ProfileListWidget(ProfileStore* store, QWidget* parent)
	: QWidget(parent), store(store)
{
	list = new QListWidget(this);
	list->setContextMenuPolicy(Qt::CustomContextMenu);
	
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(list);
	setLayout(layout);
	
	connect(store, &ProfileStore::profilesChanged, this, &ProfileListWidget::refreshList);
	connect(list, &QListWidget::itemClicked, this, &ProfileListWidget::onItemClicked);
	connect(list, &QListWidget::customContextMenuRequested, this, &ProfileListWidget::showContextMenu);

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

void ProfileListWidget::showContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = list->itemAt(pos);
    if (!item) return;

    int row = list->row(item);
    if (row < 0 || row >= store->profiles().size())
        return;

    ShuttleWindow* main = qobject_cast<ShuttleWindow*>(window());
    QMenu contextMenu;

    // Capture uniquement l'index
    int idx = row;

    const SessionProfile& p = store->profiles().at(idx);

    if (main && main->isTunnelConnected(p.name)) {
        contextMenu.addAction("Fermer le tunnel SSH", [this, idx]() {
            emit tunnelStopRequested(store->profiles().at(idx));
            });
    }
    else {
        contextMenu.addAction("Monter le tunnel SSH", [this, idx]() {
            emit tunnelStartRequested(store->profiles().at(idx));
            });
    }

    contextMenu.addAction("Ouvrir", [this, idx]() {
        emit profileSelected(store->profiles().at(idx));
        });

    contextMenu.addAction("Modifier", [this, idx]() {
        emit profileEditRequested(store->profiles().at(idx), idx);
        });

    contextMenu.addAction("Supprimer", [this, idx]() {
        emit profileDeletedRequested(idx);
        });

    contextMenu.exec(list->viewport()->mapToGlobal(pos));
}
