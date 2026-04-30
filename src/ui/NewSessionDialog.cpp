#include "NewSessionDialog.h"
#include "../ssh/AuthMethod.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QApplication>
#include <QStyle>
#include <QMessageBox>

NewSessionDialog::NewSessionDialog(QWidget* parent)
	: QDialog(parent)
{
	this->setWindowTitle("Nouvelle session SSH");
	this->resize(400, 300);

    nameEdit = new QLineEdit(this);
    hostEdit = new QLineEdit(this);
    userEdit = new QLineEdit(this);

    portSpin = new QSpinBox(this);
    portSpin->setRange(0, 65535);
    portSpin->setValue(22);

    keyPathEdit = new QLineEdit(this);
	QPushButton* browseKeyBtn = new QPushButton(this);
	browseKeyBtn->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passphraseEdit = new QLineEdit(this);
    passphraseEdit->setEchoMode(QLineEdit::Password);

	portTunnelSpin = new QSpinBox(this);
	portTunnelSpin->setRange(0, 65535);
	portTunnelSpin->setValue(0);

    createBtn = new QPushButton("Créer", this);
    cancelBtn = new QPushButton("Annuler", this);

	connect(browseKeyBtn, &QPushButton::clicked, this, &NewSessionDialog::onBrowseKeyClicked);
    connect(createBtn, &QPushButton::clicked, this, &NewSessionDialog::onCreateClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    connect(keyPathEdit, &QLineEdit::textChanged, this, &NewSessionDialog::updateAuthFields);
    connect(passphraseEdit, &QLineEdit::textChanged, this, &NewSessionDialog::updateAuthFields);
    connect(passwordEdit, &QLineEdit::textChanged, this, &NewSessionDialog::updateAuthFields);
	connect(portTunnelSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &NewSessionDialog::updateAuthFields);

	QHBoxLayout* keyLayout = new QHBoxLayout();
	keyLayout->addWidget(keyPathEdit);
	keyLayout->addWidget(browseKeyBtn);

    QFormLayout* form = new QFormLayout();
    form->addRow("Nom du profil :", nameEdit);
    form->addRow("Hôte :", hostEdit);
    form->addRow("Utilisateur :", userEdit);
    form->addRow("Port :", portSpin);
    form->addRow("Clé privée :", keyLayout);
    form->addRow("Mot de passe :", passwordEdit);
    form->addRow("Passphrase :", passphraseEdit);
    form->addRow("Port du tunnel :", portTunnelSpin);

    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(createBtn);
    buttons->addWidget(cancelBtn);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(buttons);

    setLayout(layout);

    updateAuthFields();
}

void NewSessionDialog::onCreateClicked()
{
    if (nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Le nom du profil est obligatoire.");
        return;
    }

    if (hostEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Le champ Hôte est obligatoire.");
        return;
    }

    bool tunnelEnabled = portTunnelSpin->value() != 0;
    bool hasUser = !userEdit->text().trimmed().isEmpty();
    bool hasKey = !keyPathEdit->text().isEmpty();
    bool hasPassword = !passwordEdit->text().isEmpty();

    if (tunnelEnabled) {
        if (!hasUser) {
            QMessageBox::warning(this, "Erreur", "Un nom d'utilisateur est obligatoire pour un tunnel SSH.");
            return;
        }

        if (!hasKey && !hasPassword) {
            QMessageBox::warning(this, "Erreur", "Une clé privée ou un mot de passe est obligatoire pour un tunnel SSH.");
            return;
        }
    }

    SessionProfile profile;
    profile.name = nameEdit->text();
    profile.host = hostEdit->text();
    profile.username = userEdit->text();
    profile.port = portSpin->value();
    profile.privateKeyPath = keyPathEdit->text();
    profile.password = passwordEdit->text();
    profile.passphrase = passphraseEdit->text();
	profile.portTunnel = portTunnelSpin->value();
	profile.authMethod = keyPathEdit->text().isEmpty() ? AuthMethod::Password : AuthMethod::PublicKey;

    if (editMode) {
        emit profileEdited(profile, editIndex);
	}
    else {
        emit profileCreated(profile);
    }
    accept();
}

void NewSessionDialog::onBrowseKeyClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Sélectionner une clé privée", QString(), "Clés privées (*.key *.pem *.ppk);;Tous les fichiers (*)");

    if (!filePath.isEmpty()) {
        keyPathEdit->setText(filePath);
    }
}

void NewSessionDialog::loadProfile(const SessionProfile& profile, int index)
{
	editMode = true;
    editIndex = index;

    nameEdit->setText(profile.name);
    hostEdit->setText(profile.host);
    userEdit->setText(profile.username);
    portSpin->setValue(profile.port);
    keyPathEdit->setText(profile.privateKeyPath);
    passwordEdit->setText(profile.password);
    passphraseEdit->setText(profile.passphrase);
	portTunnelSpin->setValue(profile.portTunnel);

	setWindowTitle("Modifier le profil");
    createBtn->setText("Enregistrer");
}

void NewSessionDialog::updateAuthFields()
{
    bool hasKey = !keyPathEdit->text().isEmpty() || !passphraseEdit->text().isEmpty();
    bool hasUserPass = !passwordEdit->text().isEmpty();

    // Si une clé est renseignée → désactiver user + password
    passwordEdit->setEnabled(!hasKey);

    // Si password est renseigné → désactiver clé + passphrase
    keyPathEdit->setEnabled(!hasUserPass);
    passphraseEdit->setEnabled(!hasUserPass);
}