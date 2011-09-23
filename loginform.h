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

#ifndef XDCC_LOGIN_H
#define XDCC_LOGIN_H

#include <QtGui/QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QSettings>

#include "xdcc.h"
#include "ui_xdcc_login.h"

class LoginForm : public QDialog
{
	Q_OBJECT

public:
	LoginForm(XDCC* nXDCC, QWidget *parent = 0, Qt::WFlags flags = 0);
	~LoginForm();

public slots:
	void login();
	void parseLogin(QString& data);

private:
	Ui::LoginDialog ui;

	QXmlStreamReader xml;

	QString Username;
	QSettings* settings;
	ApiFetcher* fetcher;

	XDCC* m_xDCC;
};

#endif // XDCC_H
