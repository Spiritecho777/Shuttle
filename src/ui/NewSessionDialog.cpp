#include "NewSessionDialog.h"
#include "../ssh/AuthMethod.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QApplication>
#include <QStyle>

NewSessionDialog::NewSessionDialog(QWidget* parent)
	: QDialog(parent)
{
	this->setWindowTitle("Nouvelle session SSH");
	this->resize(400, 300);

    nameEdit = new QLineEdit(this);
    hostEdit = new QLineEdit(this);
    userEdit = new QLineEdit(this);

    portSpin = new QSpinBox(this);
    portSpin->setRange(1, 65535);
    portSpin->setValue(22);

    keyPathEdit = new QLineEdit(this);
	QPushButton* browseKeyBtn = new QPushButton(this);
	browseKeyBtn->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon));
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passphraseEdit = new QLineEdit(this);
    passphraseEdit->setEchoMode(QLineEdit::Password);

    createBtn = new QPushButton("Créer", this);
    cancelBtn = new QPushButton("Annuler", this);

	connect(browseKeyBtn, &QPushButton::clicked, this, &NewSessionDialog::onBrowseKeyClicked);
    connect(createBtn, &QPushButton::clicked, this, &NewSessionDialog::onCreateClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

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

    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(createBtn);
    buttons->addWidget(cancelBtn);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(buttons);

    setLayout(layout);
}

void NewSessionDialog::onCreateClicked()
{
    SessionProfile profile;
    profile.name = nameEdit->text();
    profile.host = hostEdit->text();
    profile.username = userEdit->text();
    profile.port = portSpin->value();
    profile.privateKeyPath = keyPathEdit->text();
    profile.password = passwordEdit->text();
    profile.passphrase = passphraseEdit->text();
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

	setWindowTitle("Modifier le profil");
    createBtn->setText("Enregistrer");
}