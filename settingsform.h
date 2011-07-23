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

#ifndef XDCC_SETTINGS_H
#define XDCC_SETTINGS_H

#include <QtGui/QMainWindow>
#include <QSettings>

#include "xdcc.h"
#include "ui_xdcc_options.h"

class SettingsForm : public QDialog
{
	Q_OBJECT

public:
	SettingsForm(QWidget *parent = 0, Qt::WFlags flags = 0);
	~SettingsForm();

public slots:
	void saveSettings();

signals:
	void reloadSkin();

private:
	Ui_SettingsForm ui;
	QSettings* settings;

	bool m_SoundOnGameStart;
	bool m_FriendFollow;
	bool m_Refresh;

	QString m_Skin;
};

#endif // XDCC_H
