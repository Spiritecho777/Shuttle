#pragma once
#include "../ssh/SessionProfile.h"

#include <QWidget>
#include <QListWidget>

class ProfileStore;

class ProfileListWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ProfileListWidget(ProfileStore* store, QWidget* parent = nullptr);

signals:
	void profileSelected(const SessionProfile& profile);

private slots:
	void refreshList();
	void onItemClicked(QListWidgetItem* item);

private:
	ProfileStore* store;
	QListWidget* list;
};