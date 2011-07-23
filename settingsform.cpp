/*

   Copyright 2011 Gene Chen

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include "settingsform.h"
#include <QMessageBox>
#include <QDir>

SettingsForm::SettingsForm(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags)
{
	ui.setupUi(this);

	connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(saveSettings()));

	settings = new QSettings("DotaCash", "DCClient X");

	m_SoundOnGameStart = settings->value("GameStartedSound", true).toBool();
	ui.optionSoundGameStart->setCheckState(m_SoundOnGameStart ? Qt::Checked : Qt::Unchecked);

	m_FriendFollow = settings->value("FriendFollow", true).toBool();
	ui.optionFriendFollow->setCheckState(m_FriendFollow ? Qt::Checked : Qt::Unchecked);

	m_Refresh = settings->value("InactiveRefresh", false).toBool();
	ui.optionFriendFollow->setCheckState(m_Refresh ? Qt::Checked : Qt::Unchecked);

	m_Skin = settings->value("Skin", "default").toString();

	QDir skinsDir("./skins/");
	QStringList skinsList = skinsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	ui.cmbSkin->addItems(skinsList);

	int idx = ui.cmbSkin->findText(m_Skin);

	if(idx != -1)
		ui.cmbSkin->setCurrentIndex(idx);
}

SettingsForm::~SettingsForm()
{

}

void SettingsForm::saveSettings()
{
	m_SoundOnGameStart = ui.optionSoundGameStart->isChecked();
	settings->setValue("GameStartedSound", m_SoundOnGameStart);

	m_FriendFollow = ui.optionFriendFollow->isChecked();
	settings->setValue("FriendFollow", m_FriendFollow);

	m_Refresh = ui.optionRefresh->isChecked();
	settings->setValue("InactiveRefresh", m_Refresh);

	m_Skin = ui.cmbSkin->currentText();
	settings->setValue("Skin", m_Skin);

	QFile styleSheet(QString("./skins/%1/style.css").arg(m_Skin));
	QString style;

	if(styleSheet.open(QFile::ReadOnly))
	{
		QTextStream styleIn(&styleSheet);
		style = styleIn.readAll();
		styleSheet.close();

		QMainWindow* parent = (QMainWindow*)this->parent();
		parent->setStyleSheet(style);

		emit reloadSkin();
	}

	this->close();
}
