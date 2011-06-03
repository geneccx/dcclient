#include "gamerequester.h"
#include "gameprotocol.h"
#include "commandpacket.h"

GameRequester::GameRequester(QWidget* parent)
	: QObject(parent)
{
	socket = new QUdpSocket(this);
	socket->bind(6969, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

	connect(socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

	gameProtocol = new CGameProtocol();
}

GameRequester::~GameRequester()
{
	for(auto i = games.begin(); i != games.end(); ++i)
	{
		socket->writeDatagram(gameProtocol->SEND_W3GS_DECREATEGAME((*i)->GetEntryKey()), QHostAddress::Broadcast, 6112);
		delete *i;
	}
}