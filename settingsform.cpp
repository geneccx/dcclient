#include "settingsform.h"
#include <QMessageBox>
#include <QFileDialog>

SettingsForm::SettingsForm(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags)
{
	ui.setupUi(this);

	connect(ui.btnSave, SIGNAL(clicked()), this, SLOT(saveSettings()));

	settings = new QSettings("DotaCash", "DCClient X");

	m_SoundOnGameStart = settings->value("GameStartedSound", true).toBool();
 	ui.optionSoundGameStart->setCheckState(m_SoundOnGameStart ? Qt::Checked : Qt::Unchecked);

	m_BackgroundImage = settings->value("Background", "").toString();
	ui.txtBackground->setText(m_BackgroundImage);

	connect(ui.btnBrowse, SIGNAL(clicked()), this, SLOT(browseBackground()));
}

SettingsForm::~SettingsForm()
{

}

void SettingsForm::browseBackground()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select background image"), QString(), tr("Image Files (*.png *.jpg *.bmp)"));
	
	if(!fileName.isEmpty())
	{
		ui.txtBackground->setText(fileName);

		QMainWindow* mainWindow = (QMainWindow*)parent();
		mainWindow->setStyleSheet(QString("QWidget#centralWidget {background-image: url(%1)}").arg(fileName));
	}
}

void SettingsForm::saveSettings()
{
	m_SoundOnGameStart = ui.optionSoundGameStart->isChecked();
	settings->setValue("GameStartedSound", m_SoundOnGameStart);

	m_BackgroundImage = ui.txtBackground->text();
	settings->setValue("Background", m_BackgroundImage);

	this->close();
}