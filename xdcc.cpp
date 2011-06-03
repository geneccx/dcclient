#include "xdcc.h"

#include "irchandler.h"
#include "channelhandler.h"
#include "xmlstructs.h"
#include "dcapifetcher.h"
#include "qgproxy.h"

#include <QClipboard>
#include <QSharedPointer>

#include <winsparkle.h>

xDCC::xDCC(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(tick()));

	gproxy = new CGProxy(this);

	localServer = new QLocalServer(this);
	localServer->listen("DCClientIPC");

	connect(localServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

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
 	ui.tblPlayers->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
 	ui.tblPlayers->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);

	connect(ui.tblPubGames, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(gameDoubleClicked(int, int)));
	connect(ui.tblPrivGames, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(gameDoubleClicked(int, int)));
	connect(ui.tblHLGames, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(gameDoubleClicked(int, int)));
	connect(ui.tblCustomGames, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(gameDoubleClicked(int, int)));

	connect(ui.tblPubGames, SIGNAL(cellClicked(int, int)), this, SLOT(gameClicked(int, int)));
	connect(ui.tblPrivGames, SIGNAL(cellClicked(int, int)), this, SLOT(gameClicked(int, int)));
	connect(ui.tblHLGames, SIGNAL(cellClicked(int, int)), this, SLOT(gameClicked(int, int)));
	connect(ui.tblCustomGames, SIGNAL(cellClicked(int, int)), this, SLOT(gameClicked(int, int)));

	connect(ui.actionCheck_for_updates, SIGNAL(triggered()), this, SLOT(checkForUpdates()));
}

xDCC::~xDCC()
{
	
}

void xDCC::checkForUpdates()
{
	win_sparkle_check_update_with_ui();
}

void xDCC::gameDoubleClicked(int row, int column)
{
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

		

		ApiFetcher* safelistFetcher = new ApiFetcher(this);
		QString safelistUrl = QString("http://www.dotacash.com/api/dccsl.php?u=%1&idLobby=%2").arg(m_SessionID).arg(id);
		safelistFetcher->fetch(safelistUrl);

		gproxy->requestGame(ip);

		ui.statusBar->showMessage(tr("%1 copied to clipboard and is now visible in LAN screen.").arg(txt), 3000);
	}
}

void xDCC::gameClicked(int row, int column)
{
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
			currentID = tblItem->text();

		tick();
	}
}

void xDCC::newConnection()
{
	clientConnection = localServer->nextPendingConnection();

	connect(clientConnection, SIGNAL(disconnected()),
		clientConnection, SLOT(deleteLater()));

	connect(clientConnection, SIGNAL(readyRead()),
		this, SLOT(readData()));
}

void xDCC::readData()
{
	QDataStream in(clientConnection);

	QString arg;
	in >> arg;
	
	activateWindow();
}

void xDCC::tick()
{
	if(!currentID.isEmpty())
	{
		ApiFetcher* playersFetcher = new ApiFetcher(this);
		connect(playersFetcher, SIGNAL(fetchComplete(QString&)), this, SLOT(parsePlayersXml(QString&)));

		QString playersUrl = QString("http://www.dotacash.com/api/gp.php?u=%1&l=%2").arg(m_SessionID).arg(currentID);
		playersFetcher->fetch(playersUrl);
	}

	ApiFetcher* gamesFetcher = new ApiFetcher(this);
	connect(gamesFetcher, SIGNAL(fetchComplete(QString&)), this, SLOT(parseGamesXml(QString&)));

	currentType = ui.tabGames->currentIndex();
	QString gamesUrl = QString("http://www.dotacash.com/api/gg.php?u=%1&t=%2").arg(m_SessionID).arg(currentType+1);
	gamesFetcher->fetch(gamesUrl);

	ApiFetcher* queueFetcher = new ApiFetcher(this);
	connect(queueFetcher, SIGNAL(fetchComplete(QString&)), this, SLOT(parseQueueXml(QString&)));

	QString queueUrl = QString("http://www.dotacash.com/api/cq.php?u=%1").arg(m_SessionID);
	queueFetcher->fetch(queueUrl);
}

void xDCC::activate()
{
	ui.tabChannels->connectToIrc(this->GetUsername());
	connect(ui.tabChannels, SIGNAL(showMessage(QString&, int)), this, SLOT(showMessage(QString&, int)));

	tick();
	timer->start(3000);

	show();
}

void xDCC::showMessage(QString& message, int timeout=3000)
{
	ui.statusBar->showMessage(message, timeout);
}

void xDCC::handleChat()
{
	QString curTab = ui.tabChannels->tabText(ui.tabChannels->currentIndex());
	QString Message = ui.txtChatInput->text();
	ui.txtChatInput->clear();

	ui.tabChannels->handleChat(curTab, Message);
}

void xDCC::parseGamesXml(QString& data)
{
	QXmlStreamReader xml(data);

	for(int i = 0; i < gameInfos.size(); ++i)
		delete gameInfos.at(i);

	gameInfos.clear();

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

				gameInfos.push_back(gameInfo);

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
		switch(currentType)
		{
			case 0: table = ui.tblPubGames; break;
			case 1: table = ui.tblPrivGames; break;
			case 2: table = ui.tblHLGames; break;
			case 3: table = ui.tblCustomGames; break;
			default: return;
		}

		table->setRowCount(gameInfos.size());

		for(int i = 0; i < gameInfos.size(); ++i)
		{
			GameInfo* gameInfo = gameInfos.at(i);

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

void xDCC::parseQueueXml(QString& data)
{
	QXmlStreamReader xml(data);

	QMap<QString, QString> resultMap;
	QString currentTag;

	for(int i = 0; i < queueInfos.size(); ++i)
		delete queueInfos.at(i);

	queueInfos.clear();

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

				queueInfos.push_back(queueInfo);

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
		ui.tblQueue->setRowCount(queueInfos.size());

		for(int i = 0; i < queueInfos.size(); ++i)
		{
			QueueInfo* queueInfo = queueInfos.at(i);
			QTableWidgetItem *itemName = new QTableWidgetItem( queueInfo->name );

			itemName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemName->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			ui.tblQueue->setItem(i, 0, itemName);
		}
	}
}

void xDCC::parsePlayersXml(QString& data)
{
	QXmlStreamReader xml(data);

	QMap<QString, QString> resultMap;
	QString currentTag;

	for(int i = 0; i < playerInfos.size(); ++i)
		delete playerInfos.at(i);

	playerInfos.clear();

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

				playerInfos.push_back(playerInfo);

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
		ui.tblPlayers->setRowCount(playerInfos.size());
	
		for(int i = 0; i < playerInfos.size(); ++i)
		{
			PlayerInfo* playerInfo = playerInfos.at(i);
			QTableWidgetItem *itemName = new QTableWidgetItem( playerInfo->name );
			QTableWidgetItem *itemRealm = new QTableWidgetItem( playerInfo->realm );
			QTableWidgetItem *itemELO = new QTableWidgetItem( playerInfo->elo );

			itemName->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			//itemName->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
			itemRealm->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemRealm->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
			itemELO->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			itemELO->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

			ui.tblPlayers->setItem(i, 0, itemName);
			ui.tblPlayers->setItem(i, 1, itemRealm);
			ui.tblPlayers->setItem(i, 2, itemELO);
		}
	}
}