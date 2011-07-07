#include "xdcc.h"
#include "xdcc_version.h"

#include "irchandler.h"
#include "channelhandler.h"
#include "xmlstructs.h"
#include "dcapifetcher.h"
#include "qgproxy.h"
#include "loginform.h"
#include "settingsform.h"

#include <QClipboard>
#include <QSharedPointer>
#include <QImage>
#include <QSettings>
#include <QFile>
#include <QEvent>

#include <winsparkle.h>

XDCC::XDCC(QWidget *parent, Qt::WFlags flags) : QMainWindow(parent, flags)
{
	ui.setupUi(this);

	m_Settings = new QSettings("DotaCash", "DCClient X", this);

	m_Timer = new QTimer(this);
	connect(m_Timer, SIGNAL(timeout()), this, SLOT(tick()));

	m_CGProxy = new CGProxy(this);

	m_LocalServer = new QLocalServer(this);
	m_LocalServer->listen("DCClientIPC");

	connect(m_LocalServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

	connect(ui.txtChatInput, SIGNAL(returnPressed()), this, SLOT(handleChat()));
	connect(ui.tabGames, SIGNAL(currentChanged(int)), this, SLOT(tick()));

	ui.tblPubGames->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
	ui.tblPubGames->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed);
	ui.tblPubGames->horizontalHeader()->hideSection(2);
	ui.tblPubGames->horizontalHeader()->hideSection(3);

	ui.tblPrivGames->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
	ui.tblPrivGames->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed);
	ui.tblPrivGames->horizontalHeader()->hideSection(2);
	ui.tblPrivGames->horizontalHeader()->hideSection(3);

	ui.tblHLGames->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
	ui.tblHLGames->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed);
	ui.tblHLGames->horizontalHeader()->hideSection(2);
	ui.tblHLGames->horizontalHeader()->hideSection(3);

	ui.tblCustomGames->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
	ui.tblCustomGames->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed);
	ui.tblCustomGames->horizontalHeader()->hideSection(2);
	ui.tblCustomGames->horizontalHeader()->hideSection(3);

	ui.tblPlayers->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	ui.tblPlayers->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);
	ui.tblPlayers->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
	ui.tblPlayers->horizontalHeader()->setResizeMode(3, QHeaderView::ResizeToContents);
	ui.tblPlayers->horizontalHeader()->setResizeMode(4, QHeaderView::ResizeToContents);
	ui.tblPlayers->horizontalHeader()->setResizeMode(5, QHeaderView::ResizeToContents);

	connect(ui.tblPubGames, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(gameDoubleClicked(int, int)));
	connect(ui.tblPrivGames, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(gameDoubleClicked(int, int)));
	connect(ui.tblHLGames, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(gameDoubleClicked(int, int)));
	connect(ui.tblCustomGames, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(gameDoubleClicked(int, int)));

	connect(ui.tblPubGames, SIGNAL(cellClicked(int, int)), this, SLOT(gameClicked(int, int)));
	connect(ui.tblPrivGames, SIGNAL(cellClicked(int, int)), this, SLOT(gameClicked(int, int)));
	connect(ui.tblHLGames, SIGNAL(cellClicked(int, int)), this, SLOT(gameClicked(int, int)));
	connect(ui.tblCustomGames, SIGNAL(cellClicked(int, int)), this, SLOT(gameClicked(int, int)));

	connect(ui.actionCheck_for_updates, SIGNAL(triggered()), this, SLOT(checkForUpdates()));
	connect(ui.action_About, SIGNAL(triggered()), this, SLOT(showAbout()));
	connect(ui.action_Options, SIGNAL(triggered()), this, SLOT(showSettings()));

#ifdef WIN32
	win_sparkle_init();
#endif

	m_LoginForm = new LoginForm(this);
	m_LoginForm->show();

	m_SettingsForm = new SettingsForm(this);

	connect(m_CGProxy, SIGNAL(joinedGame(QString, QString)), ui.tabChannels, SLOT(joinedGame(QString, QString)));

	m_Skin = m_Settings->value("Skin", "default").toString();

	QFile styleSheet(QString("./skins/%1/style.css").arg(m_Skin));
	QString style;

	if(styleSheet.open(QFile::ReadOnly))
	{
		QTextStream styleIn(&styleSheet);
		style = styleIn.readAll();
		styleSheet.close();

		this->setStyleSheet(style);
	}

	m_Active = true;
	qApp->installEventFilter( this );
}

XDCC::~XDCC()
{
#ifdef WIN32
	win_sparkle_cleanup();
#endif
}

bool XDCC::eventFilter(QObject *obj,  QEvent *event)
{
	static bool inActivationEvent = false;

	if (obj == qApp && !inActivationEvent)
	{
		if (event->type() == QEvent::ApplicationActivate && !m_Active)
		{
			inActivationEvent = true;

			m_Active = true;
			this->tick();
			m_Timer->start(3000);

			inActivationEvent = false;
		}

		else if (event->type() == QEvent::ApplicationDeactivate && m_Active)
		{
			inActivationEvent = true;

			m_Active = false;
			m_Timer->stop();

			inActivationEvent = false;
		}
	}

	return QMainWindow::eventFilter(obj, event);
}

void XDCC::checkForUpdates()
{
#ifdef WIN32
	win_sparkle_check_update_with_ui();
#else
	QMessageBox::information(this, "DotaCash Client X", tr("This feature is currently disabled."));
#endif
}

void XDCC::showAbout()
{
	QMessageBox::information(this, "DotaCash Client X", tr("DotaCash Client X v%1 by Zephyrix").arg(XDCC_VERSION));
}

void XDCC::showSettings()
{
	m_SettingsForm->show();
}

void XDCC::gameDoubleClicked(int row, int column)
{
	Q_UNUSED(column);

	QTableWidget* table;
	switch(ui.tabGames->currentIndex())
	{
	case 0: table = ui.tblPubGames; break;
	case 1: table = ui.tblPrivGames; break;
	case 2: table = ui.tblHLGames; break;
	case 3: table = ui.tblCustomGames; break;
	default: return;
	}

	if(row < table->rowCount())
	{
		QString txt = table->item(row, 0)->text();
		QString id = table->item(row, 2)->text();
		QString ip = table->item(row, 3)->text();

		QClipboard *cb = QApplication::clipboard();
		cb->setText(txt);

		requestGame(ip);

		ui.statusBar->showMessage(tr("%1 copied to clipboard and is now visible in LAN screen.").arg(txt), 3000);
	}
}

void XDCC::gameClicked(int row, int column)
{
	Q_UNUSED(column);

	QTableWidget* table;
	switch(ui.tabGames->currentIndex())
	{
	case 0: table = ui.tblPubGames; break;
	case 1: table = ui.tblPrivGames; break;
	case 2: table = ui.tblHLGames; break;
	case 3: table = ui.tblCustomGames; break;
	default: return;
	}

	if(row < table->rowCount())
	{
		QTableWidgetItem* tblItem = table->item(row, 2);

		if(tblItem)
			m_CurrentID = tblItem->text();

		tick();
	}
}

void XDCC::newConnection()
{
	m_ClientConnection = m_LocalServer->nextPendingConnection();

	connect(m_ClientConnection, SIGNAL(disconnected()),
			m_ClientConnection, SLOT(deleteLater()));

	connect(m_ClientConnection, SIGNAL(readyRead()),
			this, SLOT(readData()));
}

void XDCC::readData()
{
	QDataStream in(m_ClientConnection);

	QString arg;
	in >> arg;

	activateWindow();
}

void XDCC::tick()
{
	if(!m_Active)
		return;

	if(!m_CurrentID.isEmpty())
	{
		ApiFetcher* playersFetcher = new ApiFetcher(this);
		connect(playersFetcher, SIGNAL(fetchComplete(QString&)), this, SLOT(parsePlayersXml(QString&)));

		QString playersUrl = "http://" + API_SERVER + QString("/api/gp.php?u=%1&l=%2").arg(m_SessionID).arg(m_CurrentID);
		playersFetcher->fetch(playersUrl);
	}

	ApiFetcher* gamesFetcher = new ApiFetcher(this);
	connect(gamesFetcher, SIGNAL(fetchComplete(QString&)), this, SLOT(parseGamesXml(QString&)));

	m_CurrentType = ui.tabGames->currentIndex();
	QString gamesUrl = "http://" + API_SERVER + QString("/api/gg.php?u=%1&t=%2").arg(m_SessionID).arg(m_CurrentType+1);
	gamesFetcher->fetch(gamesUrl);

	ApiFetcher* queueFetcher = new ApiFetcher(this);
	connect(queueFetcher, SIGNAL(fetchComplete(QString&)), this, SLOT(parseQueueXml(QString&)));

	QString queueUrl ="http://" + API_SERVER + QString("/api/cq.php?u=%1").arg(m_SessionID);
	queueFetcher->fetch(queueUrl);
}

void XDCC::activate()
{
	ui.tabChannels->connectToIrc(this->GetUsername());
	connect(ui.tabChannels, SIGNAL(showMessage(QString, int)), this, SLOT(showMessage(QString, int)));
	connect(m_SettingsForm, SIGNAL(reloadSkin()), ui.tabChannels, SLOT(reloadSkin()));
	connect(ui.tabChannels, SIGNAL(requestGame(QString)), this , SLOT(requestGame(QString)));

	this->tick();
	m_Timer->start(3000);

	this->show();
}

void XDCC::showMessage(QString message, int timeout=3000)
{
	ui.statusBar->showMessage(message, timeout);
}

void XDCC::handleChat()
{
	QString curTab = ui.tabChannels->tabText(ui.tabChannels->currentIndex());
	QString Message = ui.txtChatInput->text();
	ui.txtChatInput->clear();

	ui.tabChannels->handleChat(curTab, Message);
}

void XDCC::parseGamesXml(QString& data)
{
	QXmlStreamReader xml(data);

	for(int i = 0; i < m_GameInfos.size(); ++i)
		delete m_GameInfos.at(i);

	m_GameInfos.clear();

	QMap<QString, QString> resultMap;
	QString currentTag;

	while (!xml.atEnd())
	{
		xml.readNext();
		if (xml.isStartElement())
		{
			currentTag = xml.name().toString();
		}

		else if (xml.isEndElement())
		{
			if(xml.qualifiedName() == "game")
			{
				GameInfo* gameInfo = new GameInfo;

				gameInfo->name = resultMap["name"];
				gameInfo->ip = resultMap["ip"];
				gameInfo->players = resultMap["players"];
				gameInfo->id = resultMap["idLobby"];

				m_GameInfos.push_back(gameInfo);

				resultMap.clear();
			}

			currentTag.clear();
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
	else
	{

		QTableWidget* table;
		switch(m_CurrentType)
		{
		case 0: table = ui.tblPubGames; break;
		case 1: table = ui.tblPrivGames; break;
		case 2: table = ui.tblHLGames; break;
		case 3: table = ui.tblCustomGames; break;
		default: return;
		}

		table->setRowCount(m_GameInfos.size());

		for(int i = 0; i < m_GameInfos.size(); ++i)
		{
			GameInfo* gameInfo = m_GameInfos.at(i);

			QTableWidgetItem *itemName = new QTableWidgetItem( gameInfo->name );
			QTableWidgetItem *itemPlrCount = new QTableWidgetItem( gameInfo->players );
			QTableWidgetItem *idx = new QTableWidgetItem( gameInfo->id );
			QTableWidgetItem *ip = new QTableWidgetItem( gameInfo->ip );

			itemName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemPlrCount->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemPlrCount->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			table->setItem(i, 0, itemName);
			table->setItem(i, 1, itemPlrCount);
			table->setItem(i, 2, idx);
			table->setItem(i, 3, ip);
		}
	}
}

void XDCC::parseQueueXml(QString& data)
{
	QXmlStreamReader xml(data);

	QMap<QString, QString> resultMap;
	QString currentTag;

	for(int i = 0; i < m_QueueInfos.size(); ++i)
		delete m_QueueInfos.at(i);

	m_QueueInfos.clear();

	while (!xml.atEnd())
	{
		xml.readNext();
		if (xml.isStartElement())
		{
			currentTag = xml.name().toString();
		}

		else if (xml.isEndElement())
		{
			if(xml.qualifiedName() == "game")
			{
				QueueInfo* queueInfo = new QueueInfo;

				queueInfo->position = resultMap["Position"];
				queueInfo->name = resultMap["GameName"];

				m_QueueInfos.push_back(queueInfo);

				resultMap.clear();
			}

			currentTag.clear();
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
	else
	{
		ui.tblQueue->setRowCount(m_QueueInfos.size());

		for(int i = 0; i < m_QueueInfos.size(); ++i)
		{
			QueueInfo* queueInfo = m_QueueInfos.at(i);
			QTableWidgetItem *itemName = new QTableWidgetItem( queueInfo->name );

			itemName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemName->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			ui.tblQueue->setItem(i, 0, itemName);
		}
	}
}

void XDCC::parsePlayersXml(QString& data)
{
	QXmlStreamReader xml(data);

	QMap<QString, QString> resultMap;
	QString currentTag;

	for(int i = 0; i < m_PlayerInfos.size(); ++i)
		delete m_PlayerInfos.at(i);

	m_PlayerInfos.clear();

	while (!xml.atEnd())
	{
		xml.readNext();
		if (xml.isStartElement())
		{
			currentTag = xml.name().toString();
		}

		else if (xml.isEndElement())
		{
			if(xml.qualifiedName() == "player")
			{
				PlayerInfo* playerInfo = new PlayerInfo;

				playerInfo->name = resultMap["playername"];
				playerInfo->slot = resultMap["playerslot"];
				playerInfo->realm = resultMap["playerrealm"];
				playerInfo->elo = resultMap["playerelo"];
				playerInfo->games = resultMap["playergames"];
				playerInfo->kdr = resultMap["playerkdr"];
				playerInfo->wins = resultMap["playerwins"];

				m_PlayerInfos.push_back(playerInfo);

				resultMap.clear();
			}

			currentTag.clear();
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
	else
	{
		ui.tblPlayers->setRowCount(m_PlayerInfos.size());

		for(int i = 0; i < m_PlayerInfos.size(); ++i)
		{
			PlayerInfo* playerInfo = m_PlayerInfos.at(i);
			QTableWidgetItem *itemName = new QTableWidgetItem( playerInfo->name );
			QTableWidgetItem *itemRealm = new QTableWidgetItem( playerInfo->realm );
			QTableWidgetItem *itemELO = new QTableWidgetItem( playerInfo->elo );
			QTableWidgetItem *itemGames = new QTableWidgetItem( playerInfo->games );
			QTableWidgetItem *itemKDR = new QTableWidgetItem( playerInfo->kdr );
			QTableWidgetItem *itemWins = new QTableWidgetItem( playerInfo->wins );

			itemName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			//itemName->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
			itemRealm->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemRealm->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			itemELO->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemELO->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			itemGames->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemGames->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			itemKDR->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemKDR->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			itemWins->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemWins->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			QColor color;
			if(i < 5)		// sentinel
			{
				if(i % 2)
					color = QColor(0xFF, 0xEE, 0xEE);
				else
					color = QColor(0xFF, 0xF5, 0xF5);
			}
			else if(i < 10)	// scourge
			{
				if(i % 2)
					color = QColor(0xEE, 0xFF, 0xEE);
				else
					color = QColor(0xF5, 0xFF, 0xF5);
			}
			else			// other cells
				color = QColor(0xFF, 0xFF, 0xFF);

			itemName->setBackgroundColor(color);
			itemRealm->setBackgroundColor(color);
			itemELO->setBackgroundColor(color);
			itemGames->setBackgroundColor(color);
			itemKDR->setBackgroundColor(color);
			itemWins->setBackgroundColor(color);

			ui.tblPlayers->setItem(i, 0, itemName);
			ui.tblPlayers->setItem(i, 1, itemRealm);
			ui.tblPlayers->setItem(i, 2, itemELO);
			ui.tblPlayers->setItem(i, 3, itemGames);
			ui.tblPlayers->setItem(i, 4, itemKDR);
			ui.tblPlayers->setItem(i, 5, itemWins);
		}
	}
}

void XDCC::requestGame(QString IP)
{
	ApiFetcher* safelistFetcher = new ApiFetcher(this);
	QString safelistUrl = "http://" + API_SERVER + QString("/api/dccslip.php?u=%1&botip=%2").arg(m_SessionID).arg(IP);
	safelistFetcher->fetch(safelistUrl);

	m_CGProxy->requestGame(IP);
}
