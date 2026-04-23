#pragma once

#include "../core/CryptoUtils.h"
#include "AuthMethod.h"
#include "SessionProfile.h"

#include <QVector>
#include <QObject>
#include <QMessageBox>

class ProfileStore : public QObject 
{
	Q_OBJECT

public:
	explicit ProfileStore(QObject* parent = nullptr);

	const QVector<SessionProfile>& profiles() const { return m_profiles; }

	void addProfile(const SessionProfile& profile);
	void removeProfile(int index);
	void updateProfile(int index, const SessionProfile& profile);

	void load();
	void save() const;

signals:
	void profilesChanged();

private:
	QVector<SessionProfile> m_profiles;
	CryptoUtils crypto;

	void handleCorruptedFile(const QString& path);
};