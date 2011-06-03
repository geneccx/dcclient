#include "loginform.h"
#include "xdcc.h"
#include "dcapifetcher.h"

#include <QDebug>
#include <QMap>
#include <QMessageBox>

LoginForm::LoginForm(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags)
{
	ui.setupUi(this);

	connect(ui.btnLogin, SIGNAL(clicked()), this, SLOT(login()));

	settings = new QSettings("DotaCash", "DCClient X");

	QString oldUsername = settings->value("Username").toString();

	if(!oldUsername.isEmpty())
	{
		ui.txtUsername->setText(oldUsername);
		ui.txtPassword->setFocus();
	}
}

LoginForm::~LoginForm()
{

}

void LoginForm::login()
{
	QString Password;

	Username = ui.txtUsername->text();
	Password = ui.txtPassword->text();

	if(Username.isEmpty() || Password.isEmpty())
	{
		return;
	}

	ui.txtUsername->setReadOnly(true);
	ui.txtPassword->setReadOnly(true);
	ui.btnLogin->setDisabled(true);

	ApiFetcher* fetcher = new ApiFetcher();
	connect(fetcher, SIGNAL(fetchComplete(QString&)), this, SLOT(parseLogin(QString&)));

	QString url = QString("http://www.dotacash.com/api/login.php?u=%1&p=%2").arg(Username).arg(Password);
	fetcher->fetch(url);
}

void LoginForm::parseLogin(QString& data)
{
	ui.txtUsername->setReadOnly(false);
	ui.txtPassword->setReadOnly(false);
	ui.btnLogin->setDisabled(false);

	xml.clear();
	xml.addData(data);

	QString currentTag;
	QMap<QString, QString> resultMap;

	while (!xml.atEnd())
	{
		xml.readNext();
		if (xml.isStartElement())
		{
			currentTag = xml.name().toString();
		}

		else if (xml.isEndElement())
		{
			if(xml.name() == "user")
				break;
		}

		else if (xml.isCharacters() && !xml.isWhitespace())
		{
			resultMap[currentTag] = xml.text().toString();
		}
	}
	if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError)
	{
		qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
	}


	if(resultMap["result"] == "1")
	{
		QString AccountName	= resultMap["accountname"];
		quint32 Score		= resultMap["score"].toUInt();
		quint32 Rank		= resultMap["rank"].toUInt();
		QString SessionID	= resultMap["session"];

		settings->setValue("Username", Username);

		xdcc->SetUsername(Username);
		xdcc->SetScore(Score);
		xdcc->SetRank(Rank);
		xdcc->SetSessionID(SessionID);
		xdcc->activate();

		this->hide();
	}
	else
	{
		QMessageBox::information(NULL, "", tr("Invalid Login!"));
	}
}