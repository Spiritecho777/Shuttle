#include "NewSessionDialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>

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
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passphraseEdit = new QLineEdit(this);
    passphraseEdit->setEchoMode(QLineEdit::Password);

    createBtn = new QPushButton("Créer", this);
    cancelBtn = new QPushButton("Annuler", this);

    connect(createBtn, &QPushButton::clicked, this, &NewSessionDialog::onCreateClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    QFormLayout* form = new QFormLayout();
    form->addRow("Nom du profil :", nameEdit);
    form->addRow("Hôte :", hostEdit);
    form->addRow("Utilisateur :", userEdit);
    form->addRow("Port :", portSpin);
    form->addRow("Clé privée :", keyPathEdit);
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

    emit profileCreated(QVariant::fromValue(profile));
    accept();
}