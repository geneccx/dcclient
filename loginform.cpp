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

#include "loginform.h"
#include "xdcc.h"
#include "dcapifetcher.h"
#include "xdcc_version.h"

#include <QDebug>
#include <QMap>
#include <QMessageBox>

LoginForm::LoginForm(XDCC* nxDCC, QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags), m_xDCC(nxDCC)
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

	ui.webNews->setUrl(QUrl("http://" + API_SERVER + "/api/news.php"));
}

LoginForm::~LoginForm()
{
	delete settings;
	//delete fetcher;
}

void LoginForm::login()
{
	QString Password;

	Username = ui.txtUsername->text();
	Password = ui.txtPassword->text();

	if(Username.isEmpty() || Password.isEmpty())
		return;

	ui.txtUsername->setReadOnly(true);
	ui.txtPassword->setReadOnly(true);
	ui.btnLogin->setDisabled(true);

	fetcher = new ApiFetcher();
	connect(fetcher, SIGNAL(fetchComplete(QString&)), this, SLOT(parseLogin(QString&)));

	QString url = "http://" + API_SERVER + QString("/api/login.php?u=%1&p=%2").arg(Username).arg(Password);
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

		m_xDCC->SetUsername(AccountName);
		m_xDCC->SetScore(Score);
		m_xDCC->SetRank(Rank);
		m_xDCC->SetSessionID(SessionID);
		m_xDCC->activate();

		this->hide();
	}
	else
	{
		QMessageBox::information(this, "", QString("%1").arg(resultMap["reason"]));
	}
}
 
void LoginForm::closeEvent(QCloseEvent *event)
{
	QApplication::exit(0);
}
