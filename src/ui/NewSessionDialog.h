#pragma once

#include "../ssh/SessionProfile.h"

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVariant>

class NewSessionDialog : public QDialog
{
	Q_OBJECT;

public:
	explicit NewSessionDialog(QWidget* parent = nullptr);
	
signals:
	void profileCreated(const QVariant& profile);

private slots:
	void onCreateClicked();

private:
    QLineEdit* nameEdit;
    QLineEdit* hostEdit;
    QLineEdit* userEdit;
    QSpinBox* portSpin;
    QLineEdit* keyPathEdit;
    QLineEdit* passwordEdit;
    QLineEdit* passphraseEdit;

    QPushButton* createBtn;
    QPushButton* cancelBtn;
};
