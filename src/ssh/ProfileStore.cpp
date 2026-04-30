#include "ProfileStore.h"

#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

ProfileStore::ProfileStore(QObject* parent)
	: QObject(parent)
{
	load();
}

void ProfileStore::addProfile(const SessionProfile& profile)
{
	m_profiles.append(profile);
	save();
	emit profilesChanged();
}

void ProfileStore::removeProfile(int index)
{
	if (index < 0 || index >= m_profiles.size())
		return;

	m_profiles.removeAt(index);
	save();
	emit profilesChanged();
}

void ProfileStore::load()
{
	QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles.dat";
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly))
		return;

	QByteArray encryptedData = file.readAll();
	file.close();

	QByteArray decryptedData = crypto.decryptBytes(encryptedData);

	if (decryptedData.isEmpty()) {
		handleCorruptedFile(path);
		return;
	}

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(decryptedData, &parseError);
	if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
		handleCorruptedFile(path);
		return;
	}

	QJsonArray arr = doc.array();

	m_profiles.clear();
	for (const QJsonValue& val : arr) {
		if (!val.isObject()) continue;

		QJsonObject obj = val.toObject();
		SessionProfile profile;
		profile.name = obj["name"].toString();
		profile.host = obj["host"].toString();
		profile.username = obj["username"].toString();
		profile.port = obj["port"].toInt(22);
		profile.privateKeyPath = obj["privateKeyPath"].toString();
		profile.password = obj["password"].toString();
		profile.portTunnel = obj["portTunnel"].toInt(0);
		profile.passphrase = obj["passphrase"].toString();
		QString method = obj["authMethod"].toString("password");
		profile.authMethod = (method == "publickey") ? AuthMethod::PublicKey : AuthMethod::Password;
		
		m_profiles.append(profile);
	}
}

void ProfileStore::handleCorruptedFile(const QString& path)
{
	QFile::remove(path);
	QMessageBox::warning(nullptr,
		"Profils corrompus",
		"Le fichier de profils était corrompu ou dans un format obsolète.\n"
		"Il a été supprimé automatiquement.\n\n"
		"Vos profils SSH doivent être recréés.");
}

void ProfileStore::save() const
{
	QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/profiles.dat";
	
	QJsonArray arr;
	for (const SessionProfile& profile : m_profiles) 
	{
		QJsonObject obj;
		obj["name"] = profile.name;
		obj["host"] = profile.host;
		obj["username"] = profile.username;
		obj["port"] = profile.port;
		obj["privateKeyPath"] = profile.privateKeyPath;
		obj["password"] = profile.password;
		obj["portTunnel"] = profile.portTunnel;
		obj["passphrase"] = profile.passphrase;
		obj["authMethod"] = (profile.authMethod == AuthMethod::PublicKey)
			? "publickey" : "password";
		arr.append(obj);
	}

	QJsonDocument doc(arr);
	QByteArray plain = doc.toJson();

	QByteArray encrypted = crypto.encryptBytes(plain);
	if (encrypted.isEmpty())
		return;

	QFile file(path);
	if (file.open(QIODevice::WriteOnly))
		file.write(encrypted);
}

void ProfileStore::updateProfile(int index, const SessionProfile& profile)
{
	if (index < 0 || index >= m_profiles.size())
		return;
	m_profiles[index] = profile;
	save();
	emit profilesChanged();
}