#include "settingsform.h"
#include <QMessageBox>

SettingsForm::SettingsForm(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags)
{
	ui.setupUi(this);

	connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(saveSettings()));

	settings = new QSettings("DotaCash", "DCClient X");

	m_SoundOnGameStart = settings->value("GameStartedSound", true).toBool();
 	ui.optionSoundGameStart->setCheckState(m_SoundOnGameStart ? Qt::Checked : Qt::Unchecked);
}

SettingsForm::~SettingsForm()
{

}

void SettingsForm::saveSettings()
{
	m_SoundOnGameStart = ui.optionSoundGameStart->isChecked();
	settings->setValue("GameStartedSound", m_SoundOnGameStart);

	this->close();
}