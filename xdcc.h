#ifndef XDCC_H
#define XDCC_H

#include "ui_xdcc.h"

#include <QtGui/QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <qlocalserver.h>
#include <qlocalsocket.h>

class IrcHandler;
class GameRequester;
class CGProxy;

struct GameInfo;
struct QueueInfo;
struct PlayerInfo;

class xDCC : public QMainWindow
{
	Q_OBJECT

public:
	xDCC(QWidget *parent = 0, Qt::WFlags flags = 0);
	~xDCC();

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

	void showMessage(QString&, int);
	void checkForUpdates();

private:
	Ui::xDCCClass ui;

	QString m_Username;
	quint32 m_Score;
	quint32 m_Rank;
	QString m_SessionID;

	QTimer* timer;
	QLocalServer* localServer;
	QLocalSocket* clientConnection;
	IrcHandler* irc;

	CGProxy* gproxy;
	QVector<GameInfo*> gameInfos;
	QVector<QueueInfo*> queueInfos;
	QVector<PlayerInfo*> playerInfos;

	int currentType;
	QString currentID;
};

#endif // XDCC_H
