#ifndef GAMEPROTOCOL_H
#define GAMEPROTOCOL_H

#include <QByteArray>
#include <QString>

//
// CGameProtocol
//

#define W3GS_HEADER_CONSTANT		247

#define GAME_NONE					0		// this case isn't part of the protocol, it's for internal use only
#define GAME_FULL					2
#define GAME_PUBLIC					16
#define GAME_PRIVATE				17

#define GAMETYPE_CUSTOM				1
#define GAMETYPE_BLIZZARD			9

#define PLAYERLEAVE_DISCONNECT		1
#define PLAYERLEAVE_LOST			7
#define PLAYERLEAVE_LOSTBUILDINGS	8
#define PLAYERLEAVE_WON				9
#define PLAYERLEAVE_DRAW			10
#define PLAYERLEAVE_OBSERVER		11
#define PLAYERLEAVE_LOBBY			13
#define PLAYERLEAVE_GPROXY			100

#define REJECTJOIN_FULL				9
#define REJECTJOIN_STARTED			10
#define REJECTJOIN_WRONGPASSWORD	27

class CGameInfo;

class CGameProtocol
{
public:
	enum Protocol
	{
		W3GS_PING_FROM_HOST		= 1,	// 0x01
		W3GS_SLOTINFOJOIN		= 4,	// 0x04
		W3GS_REJECTJOIN			= 5,	// 0x05
		W3GS_PLAYERINFO			= 6,	// 0x06
		W3GS_PLAYERLEAVE_OTHERS	= 7,	// 0x07
		W3GS_GAMELOADED_OTHERS	= 8,	// 0x08
		W3GS_SLOTINFO			= 9,	// 0x09
		W3GS_COUNTDOWN_START	= 10,	// 0x0A
		W3GS_COUNTDOWN_END		= 11,	// 0x0B
		W3GS_INCOMING_ACTION	= 12,	// 0x0C
		W3GS_CHAT_FROM_HOST		= 15,	// 0x0F
		W3GS_START_LAG			= 16,	// 0x10
		W3GS_STOP_LAG			= 17,	// 0x11
		W3GS_HOST_KICK_PLAYER	= 28,	// 0x1C
		W3GS_REQJOIN			= 30,	// 0x1E
		W3GS_LEAVEGAME			= 33,	// 0x21
		W3GS_GAMELOADED_SELF	= 35,	// 0x23
		W3GS_OUTGOING_ACTION	= 38,	// 0x26
		W3GS_OUTGOING_KEEPALIVE	= 39,	// 0x27
		W3GS_CHAT_TO_HOST		= 40,	// 0x28
		W3GS_DROPREQ			= 41,	// 0x29
		W3GS_SEARCHGAME			= 47,	// 0x2F (UDP/LAN)
		W3GS_GAMEINFO			= 48,	// 0x30 (UDP/LAN)
		W3GS_CREATEGAME			= 49,	// 0x31 (UDP/LAN)
		W3GS_REFRESHGAME		= 50,	// 0x32 (UDP/LAN)
		W3GS_DECREATEGAME		= 51,	// 0x33 (UDP/LAN)
		W3GS_CHAT_OTHERS		= 52,	// 0x34
		W3GS_PING_FROM_OTHERS	= 53,	// 0x35
		W3GS_PONG_TO_OTHERS		= 54,	// 0x36
		W3GS_MAPCHECK			= 61,	// 0x3D
		W3GS_STARTDOWNLOAD		= 63,	// 0x3F
		W3GS_MAPSIZE			= 66,	// 0x42
		W3GS_MAPPART			= 67,	// 0x43
		W3GS_MAPPARTOK			= 68,	// 0x44
		W3GS_MAPPARTNOTOK		= 69,	// 0x45 - just a guess, received this packet after forgetting to send a crc in W3GS_MAPPART (f7 45 0a 00 01 02 01 00 00 00)
		W3GS_PONG_TO_HOST		= 70,	// 0x46
		W3GS_INCOMING_ACTION2	= 72	// 0x48 - received this packet when there are too many actions to fit in W3GS_INCOMING_ACTION
	};

	static bool AssignLength( QByteArray &content );
	static bool ValidateLength( QByteArray &content );
	static QByteArray ExtractString( QDataStream& ds );
	static QByteArray DecodeStatString(QByteArray& );

	CGameInfo* RECEIVE_W3GS_GAMEINFO( QByteArray data );

	QByteArray SEND_W3GS_CHAT_FROM_HOST( quint8 fromPID, QByteArray toPIDs, quint8 flag, quint32 flagExtra, QString message );
	QByteArray SEND_W3GS_DECREATEGAME( quint32 );
};


class CGameInfo
{
public:
	static quint32 NextUniqueGameID;
public:

	CGameInfo(quint32 nProductID, quint32 nVersion, quint32 nHostCounter, quint32 nEntryKey,
		QString nGameName, QString nGamePassword, QByteArray nStatString, quint32 nSlotsTotal,
		quint32 nMapGameType, quint32 nUnknown2, quint32 nSlotsOpen, quint32 nUpTime, quint16 nPort, bool Reliable);

	quint32 GetProductID()		{ return ProductID; }
	quint32 GetVersion()		{ return Version; }
	quint32 GetHostCounter()	{ return HostCounter; }
	quint32 GetEntryKey()		{ return EntryKey; }
	QString GetGameName()		{ return GameName; }
	QString GetGamePassword()	{ return GamePassword; }
	QByteArray GetStatString()	{ return StatString; }
	quint32 GetSlotsTotal()		{ return SlotsTotal; }
	quint32 GetMapGameType()	{ return MapGameType; }
	quint32 GetSlotsOpen()		{ return SlotsOpen; }
	quint32 GetUpTime()			{ return UpTime; }
	quint16	GetPort()			{ return Port; }
	QString GetIP()				{ return IP; }
	quint32 GetUniqueGameID()	{ return UniqueGameID; }
	bool GetReliable()			{ return Reliable; }

	void SetProductID( quint32 nProductID )			{ ProductID = nProductID; }
	void SetVersion( quint32 nVersion )				{ Version = nVersion; }
	void SetHostCounter( quint32 nHostCounter )		{ HostCounter = nHostCounter; }
	void SetGameName( QString nGameName )			{ GameName = nGameName; }
	void SetGamePassword( QString nGamePassword )	{ GamePassword = nGamePassword; }
	void SetStatString( QByteArray nStatString )	{ StatString = nStatString; }
	void SetSlotsTotal( quint32 nSlotsTotal )		{ SlotsTotal = nSlotsTotal; }
	void SetMapGameType( quint32 nMapGameType )		{ MapGameType = nMapGameType; }
	void SetSlotsOpen( quint32 nSlotsOpen )			{ SlotsOpen = nSlotsOpen; }
	void SetUpTime( quint32 nUpTime )				{ UpTime = nUpTime; }
	void SetPort( quint16 nPort )					{ Port = nPort; }
	void SetIP( QString nIP )						{ IP = nIP; }

	QByteArray GetPacket(quint16);

private:
	quint32 ProductID;
	quint32	Version;
	quint32 HostCounter;
	quint32 EntryKey;
	QString GameName;
	QString GamePassword;
	QByteArray StatString;
	quint32 SlotsTotal;
	quint32 MapGameType;
	quint32 Unknown2;
	quint32 SlotsOpen;
	quint32 UpTime;
	quint16 Port;
	QString IP;

	quint32 UniqueGameID;
	bool Reliable;
};


#endif