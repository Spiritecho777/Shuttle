#pragma once

#include <QString>
#include "AuthMethod.h"

struct SessionProfile
{
    QString name;      // Nom du profil (ex: "Serveur Prod")
    QString host;      // Adresse IP ou domaine
    QString username;      // Nom d'utilisateur SSH
    int port = 22;     // Port SSH (par défaut 22)
    QString privateKeyPath; // Chemin vers la clé privée (optionnel)
    QString password;       // Mot de passe (optionnel, si pas de clé)
    int portTunnel = 0;
	QString passphrase;     // Passphrase de la clé privée (optionnel)
	AuthMethod authMethod = AuthMethod::Password; // Méthode d'authentification
};

inline bool operator==(const SessionProfile& a, const SessionProfile& b)
{
    return a.name == b.name
        && a.host == b.host
        && a.username == b.username
        && a.port == b.port
        && a.privateKeyPath == b.privateKeyPath
        && a.password == b.password
        && a.portTunnel == b.portTunnel
        && a.passphrase == b.passphrase
        && a.authMethod == b.authMethod;
}
