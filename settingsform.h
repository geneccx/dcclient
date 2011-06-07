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

private:
	Ui_SettingsForm ui;
	QSettings* settings;

	bool m_SoundOnGameStart;
	QString m_Skin;
};

#endif // XDCC_H
