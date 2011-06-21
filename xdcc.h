#ifndef XDCC_H
#define XDCC_H

#include "ui_xdcc_main.h"

#include <QtGui/QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <qlocalserver.h>
#include <qlocalsocket.h>
#include <QSettings>
#include <QStringList>

class GameRequester;
class CGProxy;
class LoginForm;
class SettingsForm;
class ApiFetcher;

struct GameInfo;
struct QueueInfo;
struct PlayerInfo;

class XDCC : public QMainWindow
{
	Q_OBJECT

public:
	XDCC(QWidget *parent = 0, Qt::WFlags flags = 0);
	~XDCC();

	void SetUsername(QString nUsername)		{ m_Username = nUsername; ui.lbl_Account->setText(m_Username); }
	void SetScore(quint32 nScore)			{ m_Score = nScore; ui.lbl_Rating->setText(QString::number(m_Score)); }
	void SetRank(quint32 nRank)				{ m_Rank = nRank; ui.lbl_Rank->setText(QString::number(m_Rank)); }
	void SetSessionID(QString nSessionID)	{ m_SessionID = nSessionID; }

	QString GetUsername()	{ return m_Username; }
	quint32 GetScore()		{ return m_Score; }
	quint32 GetRank()		{ return m_Rank; }
	QString GetSessionID()	{ return m_SessionID; }

	void activate();

public slots:
	void tick();
	void newConnection();
	void readData();
	void handleChat();

	void gameDoubleClicked(int, int);
	void gameClicked(int, int);

	void parseGamesXml(QString&);
	void parseQueueXml(QString&);
	void parsePlayersXml(QString&);

	void showMessage(QString, int);
	void checkForUpdates();
	void showAbout();
	void showSettings();

private:
	Ui::xDCCClass ui;
	LoginForm* m_LoginForm;
	SettingsForm* m_SettingsForm;

	QSettings* m_Settings;

	QString m_Username;
	quint32 m_Score;
	quint32 m_Rank;
	QString m_SessionID;

	QTimer* m_Timer;
	QLocalServer* m_LocalServer;
	QLocalSocket* m_ClientConnection;

	CGProxy* m_CGProxy;
	QVector<GameInfo*> m_GameInfos;
	QVector<QueueInfo*> m_QueueInfos;
	QVector<PlayerInfo*> m_PlayerInfos;

	int m_CurrentType;
	QString m_CurrentID;

	QString m_Skin;

	bool m_Active;

	bool eventFilter(QObject *obj,  QEvent *event);

	ApiFetcher* m_PlayersFetcher;
	ApiFetcher* m_GamesFetcher;
	ApiFetcher* m_QueueFetcher;
};

#endif // XDCC_H
