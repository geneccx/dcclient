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

	XDCC* m_xDCC;
};

#endif // XDCC_H
