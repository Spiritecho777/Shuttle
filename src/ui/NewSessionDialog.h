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

	void loadProfile(const SessionProfile& profile, int index);
	
signals:
	void profileCreated(const SessionProfile& profile);
	void profileEdited(const SessionProfile& profile, int index);

private slots:
	void onCreateClicked();
	void onBrowseKeyClicked();

private:
	bool editMode = false;
	int editIndex = -1;

    QLineEdit* nameEdit;
    QLineEdit* hostEdit;
    QLineEdit* userEdit;
    QSpinBox* portSpin;
    QLineEdit* keyPathEdit;
    QLineEdit* passwordEdit;
    QLineEdit* passphraseEdit;

    QPushButton* createBtn;
    QPushButton* cancelBtn;

	void updateAuthFields();
};
