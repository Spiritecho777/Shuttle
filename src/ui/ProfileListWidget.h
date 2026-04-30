#pragma once
#include "../ssh/SessionProfile.h"
#include "ShuttleWindow.h"

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
	void profileDeletedRequested(int index);
	void profileEditRequested(const SessionProfile& profile, int index);
	void tunnelStartRequested(const SessionProfile& profile);
	void tunnelStopRequested(const SessionProfile& profile);

private slots:
	void refreshList();
	void onItemClicked(QListWidgetItem* item);
	void showContextMenu(const QPoint& pos);

private:
	ProfileStore* store;
	QListWidget* list;
};