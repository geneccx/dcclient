#include "qgproxy.h"
#include "gpsprotocol.h"
#include "gameprotocol.h"
#include "commandpacket.h"

#include <QSettings>
#include <QSound>
#include <QtEndian>

#include <time.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifndef WIN32
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

quint32 GetTicks( )
{
#ifdef WIN32
	return timeGetTime( );
#elif __APPLE__
	uint64_t current = mach_absolute_time( );
	static mach_timebase_info_data_t info = { 0, 0 };
	// get timebase info
	if( info.denom == 0 )
		mach_timebase_info( &info );
	uint64_t elapsednano = current * ( info.numer / info.denom );
	// convert ns to ms
	return elapsednano / 1e6;
#else
	uint32_t ticks;
	struct timespec t;
	clock_gettime( 1, &t );
	ticks = t.tv_sec * 1000;
	ticks += t.tv_nsec / 1000000;
	return ticks;
#endif
}

quint32 GetTime( )
{
	return GetTicks( ) / 1000;
}


CGProxy::CGProxy(QWidget* parent) : QObject(parent), m_LocalSocket(0)
{
	m_LocalServer = new QTcpServer(this);

	m_ListenPort = 6125;
	while(!m_LocalServer->listen(QHostAddress::LocalHost, m_ListenPort))
	{
		qDebug() << tr("failed to listen on %1").arg(m_ListenPort);
		m_ListenPort++;
	}

	connect(m_LocalServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

	m_UDPSocket = new QUdpSocket(this);
	m_UDPSocket->bind(QHostAddress::LocalHost, 6969, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

	m_RequesterSocket = new QUdpSocket(this);
	m_RequesterSocket->bind(6969, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

	connect(m_RequesterSocket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));

	m_GameProtocol = new CGameProtocol();

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(40);

	m_TotalPacketsReceivedFromLocal = 0;
	m_TotalPacketsReceivedFromRemote = 0;
	m_LastConnectionAttemptTime = 0;
	m_GameIsReliable = false;
	m_GameStarted = false;
	m_LeaveGameSent = false;
	m_ActionReceived = false;
	m_Synchronized = true;
	m_ReconnectPort = 0;
	m_PID = 255;
	m_ChatPID = 255;
	m_ReconnectKey = 0;
	m_NumEmptyActions = 0;
	m_NumEmptyActionsUsed = 0;
	m_LastAckTime = 0;
	m_LastActionTime = 0;
	m_LastBroadcastTime = 0;

	m_RemoteSocket = new QTcpSocket(this);

	connect(m_RemoteSocket, SIGNAL(disconnected()),
		this, SLOT(remoteDisconnected()));
	connect(m_RemoteSocket, SIGNAL(readyRead()), this, SLOT(readServerPackets()));
}

CGProxy::~CGProxy()
{
	m_LocalServer->close();
	
	for( QVector<CGameInfo*>::iterator i = games.begin( ); i != games.end( ); ++i )
		m_UDPSocket->writeDatagram(m_GameProtocol->SEND_W3GS_DECREATEGAME((*i)->GetUniqueGameID()), QHostAddress::Broadcast, 6112);

	while( !games.empty( ) )
	{
		delete games.front( );
		games.pop_front( );
	}

	while( !m_LocalPackets.empty( ) )
	{
		delete m_LocalPackets.front( );
		m_LocalPackets.pop_front( );
	}

	while( !m_RemotePackets.empty( ) )
	{
		delete m_RemotePackets.front( );
		m_RemotePackets.pop_front( );
	}

	while( !m_PacketBuffer.empty( ) )
	{
		delete m_PacketBuffer.front( );
		m_PacketBuffer.pop_front( );
	}
}

void CGProxy::update()
{
	processLocalPackets();
	processServerPackets();

	if( !m_RemoteServerIP.isEmpty( ) &&  m_LocalSocket != NULL && m_LocalSocket->state() ==  QTcpSocket::ConnectedState  )
	{
		if( m_GameIsReliable && m_ActionReceived && GetTime( ) - m_LastActionTime >= 60 )
		{
			if( m_NumEmptyActionsUsed < m_NumEmptyActions )
			{
				SendEmptyAction( );
				m_NumEmptyActionsUsed++;
			}
			else
			{
				SendLocalChat( "GProxy++ ran out of time to reconnect, Warcraft III will disconnect soon." );
				qDebug() << ( "[GPROXY] ran out of time to reconnect" );
			}

			m_LastActionTime = GetTime( );
		}

		if( m_RemoteSocket->state() ==  QTcpSocket::ConnectedState )
		{
			if( m_GameIsReliable && m_ActionReceived && m_ReconnectPort > 0 && GetTime( ) - m_LastAckTime >= 10 )
			{
				m_RemoteSocket->write( m_GPSProtocol->SEND_GPSC_ACK( m_TotalPacketsReceivedFromRemote ) );
				m_LastAckTime = GetTime( );
			}
		}

		if( m_RemoteSocket->state() !=  QTcpSocket::ConnectingState && m_RemoteSocket->state() !=  QTcpSocket::ConnectedState )
		{
			if( m_GameIsReliable && m_ActionReceived && m_ReconnectPort > 0 )
			{
				if(GetTime( ) - m_LastConnectionAttemptTime >= 10)
				{
					SendLocalChat( "You have been disconnected from the server." );
					quint32 TimeRemaining = ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 - ( GetTime( ) - m_LastActionTime );

					if( GetTime( ) - m_LastActionTime > ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 )
						TimeRemaining = 0;

					SendLocalChat( tr("GProxy++ is attempting to reconnect... (%1 seconds remain)").arg(TimeRemaining) );
					qDebug() << ( "[GPROXY] attempting to reconnect" );
					m_RemoteSocket->reset();
					m_RemoteSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
					m_RemoteSocket->connectToHost( m_RemoteServerIP, m_ReconnectPort );
					m_LastConnectionAttemptTime = GetTime( );
				}
			}
			else
			{
				m_LocalSocket->disconnectFromHost( );
				m_RemoteServerIP.clear( );
			}
		}

		if( m_RemoteSocket->state() ==  QTcpSocket::ConnectingState )
		{
			// we are currently attempting to connect

			if( m_RemoteSocket->waitForConnected(10000) )
			{
				// the connection attempt completed

				if( m_GameIsReliable && m_ActionReceived )
				{
					// this is a reconnection, not a new connection
					// if the server accepts the reconnect request it will send a GPS_RECONNECT back requesting a certain number of packets

					SendLocalChat( "GProxy++ reconnected to the server!" );
					SendLocalChat( "==================================================" );
					qDebug() << ( "[GPROXY] reconnected to remote server" );

					// note: even though we reset the socket when we were disconnected, we haven't been careful to ensure we never queued any data in the meantime
					// therefore it's possible the socket could have data in the send buffer
					// this is bad because the server will expect us to send a GPS_RECONNECT message first
					// so we must clear the send buffer before we continue
					// note: we aren't losing data here, any important messages that need to be sent have been put in the packet buffer
					// they will be requested by the server if required

					//m_RemoteSocket->;
					m_RemoteSocket->write( m_GPSProtocol->SEND_GPSC_RECONNECT( m_PID, m_ReconnectKey, m_TotalPacketsReceivedFromRemote ) );

					// we cannot permit any forwarding of local packets until the game is synchronized again
					// this will disable forwarding and will be reset when the synchronization is complete

					m_Synchronized = false;
				}
				else
					qDebug() << ( "[GPROXY] connected to remote server" );
			}
			else if( GetTime( ) - m_LastConnectionAttemptTime >= 10 )
			{
				// the connection attempt timed out (10 seconds)

				qDebug() << ( "[GPROXY] connect to remote server timed out" );

				if( m_GameIsReliable && m_ActionReceived && m_ReconnectPort > 0 )
				{
					quint32 TimeRemaining = ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 - ( GetTime( ) - m_LastActionTime );

					if( GetTime( ) - m_LastActionTime > ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 )
						TimeRemaining = 0;

					SendLocalChat( tr("GProxy++ is attempting to reconnect... (%1 seconds remain)").arg(TimeRemaining) );
					qDebug() << ( "[GPROXY] attempting to reconnect" );
					m_RemoteSocket->connectToHost( m_RemoteServerIP, m_ReconnectPort );
					m_LastConnectionAttemptTime = GetTime( );
				}
				else
				{
					m_LocalSocket->disconnectFromHost( );
					m_RemoteServerIP.clear( );
				}
			}
		}
	}

	if(!m_LocalSocket || m_LocalSocket->state() != QTcpSocket::ConnectedState || m_RemoteSocket->state() != QTcpSocket::ConnectedState)
	{
		if( GetTime( ) - m_LastBroadcastTime > 3 )
		{
			for(QVector<CGameInfo*>::const_iterator i = games.constBegin(); i != games.constEnd(); ++i)
				m_UDPSocket->writeDatagram((*i)->GetPacket(m_ListenPort), QHostAddress::LocalHost, 6112);

			m_LastBroadcastTime = GetTime( );
		}
	}
}

void CGProxy::newConnection()
{
	QTcpSocket* newClient = m_LocalServer->nextPendingConnection();

	if(m_LocalSocket)
	{
		newClient->disconnectFromHost();
		return;
	}

	while( !m_LocalPackets.empty( ) )
	{
		delete m_LocalPackets.front( );
		m_LocalPackets.pop_front( );
	}

	while( !m_RemotePackets.empty( ) )
	{
		delete m_RemotePackets.front( );
		m_RemotePackets.pop_front( );
	}

	while( !m_PacketBuffer.empty( ) )
	{
		delete m_PacketBuffer.front( );
		m_PacketBuffer.pop_front( );
	}

	newClient->setSocketOption(QAbstractSocket::LowDelayOption, 1);

	m_TotalPacketsReceivedFromLocal = 0;
	m_TotalPacketsReceivedFromRemote = 0;
	m_LastConnectionAttemptTime = 0;
	m_GameIsReliable = false;
	m_GameStarted = false;
	m_LeaveGameSent = false;
	m_ActionReceived = false;
	m_Synchronized = true;
	m_ReconnectPort = 0;
	m_PID = 255;
	m_ChatPID = 255;
	m_ReconnectKey = 0;
	m_NumEmptyActions = 0;
	m_NumEmptyActionsUsed = 0;
	m_LastAckTime = 0;
	m_LastActionTime = 0;

	m_LocalSocket = newClient;

	connect(m_LocalSocket, SIGNAL(disconnected()),
		this, SLOT(localDisconnected()));

	connect(m_LocalSocket, SIGNAL(disconnected()),
		m_LocalSocket, SLOT(deleteLater()));

	connect(m_LocalSocket, SIGNAL(readyRead()), this, SLOT(readLocalPackets()));
}

void CGProxy::localDisconnected()
{
	if( m_GameIsReliable && !m_LeaveGameSent && m_RemoteSocket->state() == QTcpSocket::ConnectedState)
	{
		// note: we're not actually 100% ensuring the leavegame message is sent, we'd need to check that DoSend worked, etc...

		QByteArray LeaveGame;
		QDataStream ds(&LeaveGame, QIODevice::ReadWrite);
		ds.setByteOrder(QDataStream::LittleEndian);
		ds << (quint8)W3GS_HEADER_CONSTANT;
		ds << (quint8)CGameProtocol::W3GS_LEAVEGAME;
		ds << (quint16)0x08;
		ds << (quint32)PLAYERLEAVE_GPROXY;

		m_RemoteSocket->write( LeaveGame );
		m_RemoteSocket->waitForBytesWritten();

		m_LeaveGameSent = true;
	}

	m_RemoteSocket->disconnectFromHost();

	m_LocalSocket = NULL;

	m_RemoteBytes.clear();
	m_LocalBytes.clear();

	while( !m_LocalPackets.empty( ) )
	{
		delete m_LocalPackets.front( );
		m_LocalPackets.pop_front( );
	}

	while( !m_RemotePackets.empty( ) )
	{
		delete m_RemotePackets.front( );
		m_RemotePackets.pop_front( );
	}

	while( !m_PacketBuffer.empty( ) )
	{
		delete m_PacketBuffer.front( );
		m_PacketBuffer.pop_front( );
	}
}

void CGProxy::remoteDisconnected()
{
	while( !m_LocalPackets.empty( ) )
	{
		delete m_LocalPackets.front( );
		m_LocalPackets.pop_front( );
	}

	while( !m_RemotePackets.empty( ) )
	{
		delete m_RemotePackets.front( );
		m_RemotePackets.pop_front( );
	}

	if(m_LeaveGameSent)
		return;

	if(m_LocalSocket == NULL || m_LocalSocket->state() !=  QTcpSocket::ConnectedState)
	{
		return;
	}

	qDebug() << ( "[GPROXY] disconnected from remote server due to socket error" );

	if( m_GameIsReliable && m_ActionReceived && m_ReconnectPort > 0 )
	{
		SendLocalChat( "You have been disconnected from the server due to a socket error." );
		quint32 TimeRemaining = ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 - ( GetTime( ) - m_LastActionTime );

		if( GetTime( ) - m_LastActionTime > ( m_NumEmptyActions - m_NumEmptyActionsUsed + 1 ) * 60 )
			TimeRemaining = 0;

		SendLocalChat( tr("GProxy++ is attempting to reconnect... (%1 seconds remain)").arg(TimeRemaining) );
		qDebug() << ( "[GPROXY] attempting to reconnect" );

		m_RemoteSocket->reset();
		m_RemoteSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
		m_RemoteSocket->connectToHost( m_RemoteServerIP, m_ReconnectPort );
		m_LastConnectionAttemptTime = GetTime( );
	}
	else
	{
		m_LocalSocket->disconnectFromHost( );
		m_RemoteServerIP.clear( );
	}
}

void CGProxy::readLocalPackets()
{
	QByteArray bytes = m_LocalSocket->readAll();
	m_LocalBytes += bytes;

	while( m_LocalBytes.size( ) >= 4 )
	{
		if( m_LocalBytes.at(0) == W3GS_HEADER_CONSTANT )
		{
			QByteArray LengthBytes;
			LengthBytes.push_back( m_LocalBytes[2] );
			LengthBytes.push_back( m_LocalBytes[3] );
			quint16 length = qFromLittleEndian<quint16>((uchar*)LengthBytes.data());
			
			if(length >= 4)
			{
				if( m_LocalBytes.size( ) >= length )
				{
					QByteArray data = QByteArray(m_LocalBytes, length);
					QDataStream ds(data);
	
					bool forward = true;

					if( m_LocalBytes.at(1) == CGameProtocol :: W3GS_CHAT_TO_HOST )
					{
						if( data.size( ) >= 5 )
						{
							unsigned int i = 5;
							unsigned char Total = data[4];

							if( Total > 0 && data.size( ) >= i + Total )
							{
								i += Total;
								unsigned char Flag = data[i + 1];
								i += 2;

								QString MessageString;

								if( Flag == 16 && data.size( ) >= i + 1 )
								{
									ds.skipRawData(i);
									MessageString = CGameProtocol :: ExtractString(ds);
								}
								else if( Flag == 32 && data.size( ) >= i + 5 )
								{
									ds.skipRawData(i+4);
									MessageString = CGameProtocol :: ExtractString(ds);
								}

								QString Command = MessageString.toLower();

								if( Command.size( ) >= 1 && Command.at(0) == '/' )
								{
									forward = false;

									if( Command.size( ) >= 5 && Command.mid( 0, 4 ) == "/re " )
									{
// 										if( m_BNET->GetLoggedIn( ) )
// 										{
// 											//if( !m_BNET->GetReplyTarget( ).empty( ) )
// 											{
// 												//m_BNET->QueueChatCommand( MessageString.substr( 4 ), m_BNET->GetReplyTarget( ), true );
// 												SendLocalChat( "Whispered to " + m_BNET->GetReplyTarget( ) + ": " + MessageString.substr( 4 ) );
// 											}
// 											else
// 												SendLocalChat( "Nobody has whispered you yet." );
// 										}
// 										else
// 											SendLocalChat( "You are not connected to battle.net." );
									}
									else if( Command == "/status" )
									{
										if( m_LocalSocket )
										{
											if( m_GameIsReliable && m_ReconnectPort > 0 )
												SendLocalChat( "GProxy++ disconnect protection: enabled" );
											else
												SendLocalChat( "GProxy++ disconnect protection: disabled" );
										}
									}
								}
							}
						}
					}

					if( forward )
					{
						m_LocalPackets.push_back( new CCommandPacket( W3GS_HEADER_CONSTANT, m_LocalBytes[1], data ) );
						m_PacketBuffer.push_back( new CCommandPacket( W3GS_HEADER_CONSTANT, m_LocalBytes[1], data ) );
						m_TotalPacketsReceivedFromLocal++;
					}

					m_LocalBytes = QByteArray(m_LocalBytes.begin( ) + length, m_LocalBytes.size() - length);
				}
				else
					return;
			}
			else
			{
				qDebug() << "[GPROXY] received invalid packet from local player (bad length)";
				m_LeaveGameSent = true;
				m_LocalSocket->disconnectFromHost( );
				return;
			}
		}
		else
		{
			qDebug() << ( "[GPROXY] received invalid packet from local player (bad header constant)" );
			m_LeaveGameSent = true;
			m_LocalSocket->disconnectFromHost( );
			return;
		}
	}

	processLocalPackets();
}

void CGProxy::processLocalPackets()
{	
	if(!m_LocalSocket || m_LocalSocket->state() != QTcpSocket::ConnectedState)
		return;

	while( !m_LocalPackets.empty( ) )
	{
		CCommandPacket *Packet = m_LocalPackets.front( );
		m_LocalPackets.pop_front( );
		QByteArray Data = Packet->GetData( );
		QDataStream ds(Data);
		ds.setByteOrder(QDataStream::LittleEndian);

		if( Packet->GetPacketType( ) == W3GS_HEADER_CONSTANT )
		{
			if( Packet->GetID( ) == CGameProtocol :: W3GS_REQJOIN )
			{
				if( Data.size( ) >= 20 )
				{
					quint32 HostCounter;
					quint32 EntryKey;
					quint8 Unknown;
					quint16 ListenPort;
					quint32 PeerKey;
					QString Name;
					QByteArray Remainder;

					ds.skipRawData(4);
					ds >> HostCounter;
					ds >> EntryKey;
					ds >> Unknown;
					ds >> ListenPort;
					ds >> PeerKey;
					Name = CGameProtocol::ExtractString(ds);
					Remainder = QByteArray(Data.begin() + Name.size() + 20, Data.size() - (Name.size() + 20));

					if(Remainder.size( ) == 18)
					{
						bool GameFound = false;

						for(QVector<CGameInfo*>::iterator i = games.begin( ); i != games.end( ); ++i)
						{
							if((*i)->GetUniqueGameID() == EntryKey)
							{
								m_RemoteSocket->reset();
								m_RemoteSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
								m_RemoteSocket->connectToHost((*i)->GetIP(), (*i)->GetPort());
								if(!m_RemoteSocket->waitForConnected(3000))
								{
									qDebug() << "[GPROXY] player requested to join an expired game. removing from list.";
									m_UDPSocket->writeDatagram(m_GameProtocol->SEND_W3GS_DECREATEGAME((*i)->GetUniqueGameID()), QHostAddress::Broadcast, 6112);
									delete (*i);
									games.erase(i);
									break;
								}

								m_LastConnectionAttemptTime = GetTime();
								m_RemoteServerIP = (*i)->GetIP();
								m_GameIsReliable = (*i)->GetReliable();
								m_GameStarted = false;

								QByteArray DataRewritten;
								QDataStream ds(&DataRewritten, QIODevice::ReadWrite);
								ds.setByteOrder(QDataStream::LittleEndian);

								ds << (quint8)W3GS_HEADER_CONSTANT;
								ds << (quint8)Packet->GetID();
								ds << (quint16)0;
								ds << (*i)->GetHostCounter();
								ds << (*i)->GetEntryKey();
								ds << (quint8)Unknown;
								ds << (quint16)ListenPort;
								ds << (quint32)PeerKey;
								ds.writeRawData(Name.toUtf8(), Name.length());
								ds << (quint8)0;
								ds.writeRawData(Remainder.data(), Remainder.length());

								CGameProtocol::AssignLength(DataRewritten);

								Data = DataRewritten;

								GameFound = true;
								break;
							}
						}

						if(!GameFound)
						{
							qDebug() << "[GPROXY] local player requested unknown game (expired?) sending decreate.";
							m_UDPSocket->writeDatagram(m_GameProtocol->SEND_W3GS_DECREATEGAME(EntryKey), QHostAddress::Broadcast, 6112);
							m_LocalSocket->disconnectFromHost();
						}
					}
					else
						qDebug() << "[GPROXY] received invalid join request from local player (invalid remainder)";
				}
				else
					qDebug() << "[GPROXY] received invalid join request from local player (too short)";
			}
			else if(Packet->GetID( ) == CGameProtocol :: W3GS_LEAVEGAME)
			{
				QByteArray LeaveGame;
				QDataStream ds(&LeaveGame, QIODevice::ReadWrite);
				ds.setByteOrder(QDataStream::LittleEndian);
				ds << (quint8)W3GS_HEADER_CONSTANT;
				ds << (quint8)CGameProtocol::W3GS_LEAVEGAME;
				ds << (quint16)0x08;
				ds << (quint32)0x00;

				m_RemoteSocket->write( LeaveGame );
				m_RemoteSocket->waitForBytesWritten();

				m_LeaveGameSent = true;
				m_LocalSocket->disconnectFromHost();
			}
		}

		if( m_RemoteSocket && m_Synchronized )
			m_RemoteSocket->write(Data);

		delete Packet;
	}
}

void CGProxy::readServerPackets()
{
	QByteArray bytes = m_RemoteSocket->readAll();
	m_RemoteBytes += bytes;

	while( m_RemoteBytes.size( ) >= 4 )
	{
		if( m_RemoteBytes.at(0) == W3GS_HEADER_CONSTANT || m_RemoteBytes.at(0) == GPS_HEADER_CONSTANT )
		{
			QByteArray LengthBytes;
			LengthBytes.push_back( m_RemoteBytes[2] );
			LengthBytes.push_back( m_RemoteBytes[3] );
			quint16 length = qFromLittleEndian<quint16>((uchar*)LengthBytes.data());
			if( length >= 4 )
			{
				if( m_RemoteBytes.size( ) >= length )
				{
				
					QByteArray data = QByteArray(m_RemoteBytes.begin(), length);
					m_RemotePackets.push_back(new CCommandPacket( m_RemoteBytes.at(0), m_RemoteBytes.at(1), data ));

					if( m_RemoteBytes.at(0) == W3GS_HEADER_CONSTANT )
						m_TotalPacketsReceivedFromRemote++;

					m_RemoteBytes = QByteArray(m_RemoteBytes.begin( ) + length, m_RemoteBytes.size() - length);
				}
				else
					return;
			}
			else
			{
				qDebug() << "[GPROXY] received invalid packet from remote server (bad length)";
				m_RemoteSocket->disconnectFromHost();
				return;
			}
		}
		else
		{
			qDebug() << "[GPROXY] received invalid packet from remote server (bad header constant)";
			m_RemoteSocket->disconnectFromHost();
			return;
		}
	}

	processServerPackets();
}

void CGProxy::processServerPackets()
{
	if(!m_LocalSocket || m_LocalSocket->state() != QTcpSocket::ConnectedState || m_RemoteSocket->state() != QTcpSocket::ConnectedState)
		return;

	while( !m_RemotePackets.empty( ) )
	{
		CCommandPacket *Packet = m_RemotePackets.front( );
		m_RemotePackets.pop_front( );
		QByteArray Data = Packet->GetData( );
		QDataStream ds(Data);
		ds.setByteOrder(QDataStream::LittleEndian);
		ds.skipRawData(4);

		if( Packet->GetPacketType( ) == W3GS_HEADER_CONSTANT )
		{
			if( Packet->GetID( ) == CGameProtocol :: W3GS_SLOTINFOJOIN )
			{
				if( Data.size( ) >= 6 )
				{
					quint16 SlotInfoSize;
					ds >> SlotInfoSize;

					if( Data.size( ) >= 7 + SlotInfoSize )
						m_ChatPID = Data[6 + SlotInfoSize];
				}

				// send a GPS_INIT packet
				// if the server doesn't recognize it (e.g. it isn't GHost++) we should be kicked

				qDebug() << ( "[GPROXY] join request accepted by remote server" );

				if( m_GameIsReliable )
				{
					qDebug() << ( "[GPROXY] detected reliable game, starting GPS handshake" );
					m_RemoteSocket->write( m_GPSProtocol->SEND_GPSC_INIT( 1 ) );
				}
				else
					qDebug() << ( "[GPROXY] detected standard game, disconnect protection disabled" );
			}
			else if( Packet->GetID( ) == CGameProtocol :: W3GS_COUNTDOWN_END )
			{
				if( m_GameIsReliable && m_ReconnectPort > 0 )
					qDebug() << ( "[GPROXY] game started, disconnect protection enabled" );
				else
				{
					if( m_GameIsReliable )
						qDebug() << ( "[GPROXY] game started but GPS handshake not complete, disconnect protection disabled" );
					else
						qDebug() << ( "[GPROXY] game started" );
				}

				for( QVector<CGameInfo*>::iterator i = games.begin( ); i != games.end( ); ++i )
					m_UDPSocket->writeDatagram(m_GameProtocol->SEND_W3GS_DECREATEGAME((*i)->GetUniqueGameID()), QHostAddress::Broadcast, 6112);

				while( !games.empty( ) )
				{
					delete games.front( );
					games.pop_front( );
				}

				QSettings settings("DotaCash", "DCClient X");

				if(settings.value("GameStartedSound", true).toBool())
					QSound::play("./sounds/GameStarted.wav");

				m_GameStarted = true;
			}
			else if( Packet->GetID( ) == CGameProtocol :: W3GS_INCOMING_ACTION )
			{
				if( m_GameIsReliable )
				{
					// we received a game update which means we can reset the number of empty actions we have to work with
					// we also must send any remaining empty actions now
					// note: the lag screen can't be up right now otherwise the server made a big mistake, so we don't need to check for it

					QByteArray EmptyAction;
					EmptyAction.push_back( 0xF7 );
					EmptyAction.push_back( 0x0C );
					EmptyAction.push_back( 0x06 );
					EmptyAction.push_back( (char)0x00 );
					EmptyAction.push_back( (char)0x00 );
					EmptyAction.push_back( (char)0x00 );

					for( unsigned char i = m_NumEmptyActionsUsed; i < m_NumEmptyActions; i++ )
						m_LocalSocket->write( EmptyAction );

					m_NumEmptyActionsUsed = 0;
				}

				m_ActionReceived = true;
				m_LastActionTime = GetTime( );
			}
			else if( Packet->GetID( ) == CGameProtocol :: W3GS_START_LAG )
			{
				if( m_GameIsReliable )
				{
					QByteArray Data = Packet->GetData( );

					if( Data.size( ) >= 5 )
					{
						quint8 NumLaggers = Data[4];

						if( Data.size( ) == 5 + NumLaggers * 5 )
						{
							for( quint8 i = 0; i < NumLaggers; i++ )
							{
								bool LaggerFound = false;

								for( QVector<quint8>::iterator j = m_Laggers.begin( ); j != m_Laggers.end( ); j++ )
								{
									if( *j == Data[5 + i * 5] )
										LaggerFound = true;
								}

								if( LaggerFound )
									qDebug() << ( "[GPROXY] warning - received start_lag on known lagger" );
								else
									m_Laggers.push_back( Data[5 + i * 5] );
							}
						}
						else
							qDebug() << ( "[GPROXY] warning - unhandled start_lag (2)" );
					}
					else
						qDebug() << ( "[GPROXY] warning - unhandled start_lag (1)" );
				}
			}
			else if( Packet->GetID( ) == CGameProtocol :: W3GS_STOP_LAG )
			{
				if( m_GameIsReliable )
				{
					QByteArray Data = Packet->GetData( );

					if( Data.size( ) == 9 )
					{
						bool LaggerFound = false;

						for( QVector<quint8>::iterator i = m_Laggers.begin( ); i != m_Laggers.end( ); )
						{
							if( *i == Data[4] )
							{
								i = m_Laggers.erase( i );
								LaggerFound = true;
							}
							else
								i++;
						}

						if( !LaggerFound )
							qDebug() << ( "[GPROXY] warning - received stop_lag on unknown lagger" );
					}
					else
						qDebug() << ( "[GPROXY] warning - unhandled stop_lag" );
				}
			}
			else if( Packet->GetID( ) == CGameProtocol :: W3GS_INCOMING_ACTION2 )
			{
				if( m_GameIsReliable )
				{
					// we received a fractured game update which means we cannot use any empty actions until we receive the subsequent game update
					// we also must send any remaining empty actions now
					// note: this means if we get disconnected right now we can't use any of our buffer time, which would be very unlucky
					// it still gives us 60 seconds total to reconnect though
					// note: the lag screen can't be up right now otherwise the server made a big mistake, so we don't need to check for it

					QByteArray EmptyAction;
					EmptyAction.push_back( 0xF7 );
					EmptyAction.push_back( 0x0C );
					EmptyAction.push_back( 0x06 );
					EmptyAction.push_back( (char)0x00 );
					EmptyAction.push_back( (char)0x00 );
					EmptyAction.push_back( (char)0x00 );

					for( unsigned char i = m_NumEmptyActionsUsed; i < m_NumEmptyActions; i++ )
						m_LocalSocket->write( EmptyAction );

					m_NumEmptyActionsUsed = m_NumEmptyActions;
				}
			}

			// forward the data

			m_LocalSocket->write( Packet->GetData( ) );

			// we have to wait until now to send the status message since otherwise the slotinfojoin itself wouldn't have been forwarded

			if( Packet->GetID( ) == CGameProtocol :: W3GS_SLOTINFOJOIN )
			{
				if( m_GameIsReliable )
					SendLocalChat( tr("This is a reliable game. Requesting GProxy++ disconnect protection from server...") );
				else
					SendLocalChat( tr("This is an unreliable game. GProxy++ disconnect protection is disabled.") );
			}
		}
		else if( Packet->GetPacketType( ) == GPS_HEADER_CONSTANT )
		{
			if( m_GameIsReliable )
			{
				QByteArray Data = Packet->GetData( );

				if( Packet->GetID( ) == CGPSProtocol :: GPS_INIT && Data.size( ) == 12 )
				{
					ds >> m_ReconnectPort;
					ds >> m_PID;
					ds >> m_ReconnectKey;
					ds >> m_NumEmptyActions;
					SendLocalChat( tr("GProxy++ disconnect protection is ready (%1 second buffer).").arg( ( m_NumEmptyActions + 1 ) * 60 ) );
					qDebug() << tr("[GPROXY] GProxy++ disconnect protection is ready (%1 second buffer).").arg( ( m_NumEmptyActions + 1 ) * 60 );
				}
				else if( Packet->GetID( ) == CGPSProtocol :: GPS_RECONNECT && Data.size( ) == 8 )
				{
					quint32 LastPacket;
					ds >> LastPacket;
					quint32 PacketsAlreadyUnqueued = m_TotalPacketsReceivedFromLocal - m_PacketBuffer.size( );

					if( LastPacket > PacketsAlreadyUnqueued )
					{
						quint32 PacketsToUnqueue = LastPacket - PacketsAlreadyUnqueued;

						if( PacketsToUnqueue > m_PacketBuffer.size( ) )
						{
							qDebug() << ( "[GPROXY] received GPS_RECONNECT with last packet > total packets sent" );
							PacketsToUnqueue = m_PacketBuffer.size( );
						}

						while( PacketsToUnqueue > 0 )
						{
							delete m_PacketBuffer.front( );
							m_PacketBuffer.pop_front( );
							PacketsToUnqueue--;
						}
					}

					// send remaining packets from buffer, preserve buffer
					// note: any packets in m_LocalPackets are still sitting at the end of this buffer because they haven't been processed yet
					// therefore we must check for duplicates otherwise we might (will) cause a desync

					QQueue<CCommandPacket *> TempBuffer;

					while( !m_PacketBuffer.empty( ) )
					{
						if( m_PacketBuffer.size( ) > m_LocalPackets.size( ) )
							m_RemoteSocket->write( m_PacketBuffer.front( )->GetData( ) );

						TempBuffer.push_back( m_PacketBuffer.front( ) );
						m_PacketBuffer.pop_front( );
					}

					m_PacketBuffer = TempBuffer;

					// we can resume forwarding local packets again
					// doing so prior to this point could result in an out-of-order stream which would probably cause a desync

					m_Synchronized = true;
				}
				else if( Packet->GetID( ) == CGPSProtocol :: GPS_ACK && Data.size( ) == 8 )
				{
					quint32 LastPacket;
					ds >> LastPacket;
					quint32 PacketsAlreadyUnqueued = m_TotalPacketsReceivedFromLocal - m_PacketBuffer.size( );

					if( LastPacket > PacketsAlreadyUnqueued )
					{
						quint32 PacketsToUnqueue = LastPacket - PacketsAlreadyUnqueued;

						if( PacketsToUnqueue > m_PacketBuffer.size( ) )
						{
							qDebug() << ( "[GPROXY] received GPS_ACK with last packet > total packets sent" );
							PacketsToUnqueue = m_PacketBuffer.size( );
						}

						while( PacketsToUnqueue > 0 )
						{
							delete m_PacketBuffer.front( );
							m_PacketBuffer.pop_front( );
							PacketsToUnqueue--;
						}
					}
				}
				else if( Packet->GetID( ) == CGPSProtocol :: GPS_REJECT && Data.size( ) == 8 )
				{
					quint32 Reason;
					ds >> Reason;

					if( Reason == REJECTGPS_INVALID )
						qDebug() << ( "[GPROXY] rejected by remote server: invalid data" );
					else if( Reason == REJECTGPS_NOTFOUND )
						qDebug() << ( "[GPROXY] rejected by remote server: player not found in any running games" );

					m_LocalSocket->disconnectFromHost( );
				}
			}
		}

		delete Packet;
	}
}

void CGProxy::requestGame(QString host)
{
	m_RequesterSocket->writeDatagram(QByteArray("rcon_broadcast"), QHostAddress(host), 6969);
}

void CGProxy::readPendingDatagrams()
{
	while (m_RequesterSocket->hasPendingDatagrams())
	{
		QByteArray datagram;
		datagram.resize(m_RequesterSocket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		m_RequesterSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

		parsePacket(sender.toString(), datagram);
	}
}

void CGProxy::parsePacket(QString IP, QByteArray datagram)
{
	if( datagram.at(0) == W3GS_HEADER_CONSTANT )
	{
		if( datagram.at(1) == CGameProtocol :: W3GS_GAMEINFO )
		{
			CGameInfo* gameInfo = m_GameProtocol->RECEIVE_W3GS_GAMEINFO(datagram);
			gameInfo->SetIP(IP);

			bool DuplicateFound = false;

			for(QVector<CGameInfo*>::iterator i = games.begin(); i != games.end(); ++i)
			{
				if((*i)->GetIP() == IP && (*i)->GetPort() == gameInfo->GetPort())
				{
					m_UDPSocket->writeDatagram(m_GameProtocol->SEND_W3GS_DECREATEGAME((*i)->GetUniqueGameID()), QHostAddress::Broadcast, 6112);
					delete *i;
					*i = gameInfo;
					DuplicateFound = true;
					break;
				}
			}

			if(!DuplicateFound)
			{
				games.push_back(gameInfo);

				for(QVector<CGameInfo*>::const_iterator i = games.constBegin(); i != games.constEnd(); ++i)
					m_UDPSocket->writeDatagram((*i)->GetPacket(m_ListenPort), QHostAddress::LocalHost, 6112);

				m_LastBroadcastTime = GetTime( );
			}
		}
	}

}

void CGProxy::SendLocalChat( QString message )
{
	QByteArray toPIDs;
	toPIDs.append(m_ChatPID);

	if( m_LocalSocket )
	{
		if( m_GameStarted )
		{
			if( message.size( ) > 127 )
				message = message.left(127);

			m_LocalSocket->write( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( m_ChatPID, toPIDs, 32, 0, message ) );
		}
		else
		{
			if( message.size( ) > 254 )
				message = message.left(254);

			m_LocalSocket->write( m_GameProtocol->SEND_W3GS_CHAT_FROM_HOST( m_ChatPID, toPIDs, 16, -1, message ) );
		}
	}
}

void CGProxy::SendEmptyAction( )
{
	// we can't send any empty actions while the lag screen is up
	// so we keep track of who the lag screen is currently showing (if anyone) and we tear it down, send the empty action, and put it back up

	for( QVector<quint8>::iterator i = m_Laggers.begin( ); i != m_Laggers.end( ); i++ )
	{
		QByteArray StopLag;
		QDataStream ds(&StopLag, QIODevice::ReadWrite);
		ds.setByteOrder(QDataStream::LittleEndian);

		ds << (quint8)W3GS_HEADER_CONSTANT;
		ds << (quint8)CGameProtocol::W3GS_STOP_LAG;
		ds << (quint16)9;
		ds << (quint8)0;
		ds << (quint8)*i;
		ds << (quint32)60000;

		m_LocalSocket->write( StopLag );
	}

	QByteArray EmptyAction;

	EmptyAction.push_back( 0xF7 );
	EmptyAction.push_back( 0x0C );
	EmptyAction.push_back( 0x06 );
	EmptyAction.push_back( (char)0x00 );
	EmptyAction.push_back( (char)0x00 );
	EmptyAction.push_back( (char)0x00 );
	m_LocalSocket->write( EmptyAction );

	if( !m_Laggers.empty( ) )
	{
		QByteArray StartLag;
		QDataStream ds(&StartLag, QIODevice::ReadWrite);
		ds.setByteOrder(QDataStream::LittleEndian);

		ds << (quint8)W3GS_HEADER_CONSTANT;
		ds << (quint8)CGameProtocol::W3GS_START_LAG;
		ds << (quint16)0;
		ds << (quint8)m_Laggers.size();

		for( QVector<quint8>::iterator i = m_Laggers.begin( ); i != m_Laggers.end( ); i++ )
		{
			// using a lag time of 60000 ms means the counter will start at zero
			// hopefully warcraft 3 doesn't care about wild variations in the lag time in subsequent packets

			ds << (quint8)*i;
			ds << (quint32)60000;
		}

		CGameProtocol::AssignLength(StartLag);

		m_LocalSocket->write( StartLag );
	}
}
