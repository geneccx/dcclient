/*

   Copyright 2011 Trevor Hogan

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
   Modified by Gene Chen for Qt compatibility

*/

#include "gameprotocol.h"
#include <QtEndian>
#include <QDataStream>
#include <QDebug>

QByteArray CGameProtocol :: ExtractString( QDataStream& ds )
{
	QByteArray data; // will hold your data when done
	quint8 byte;
	do
	{
		// ds is your QDataStream already pointing into your file
		ds >> byte;
		data.append(byte);
	} while (byte != 0);

	return data;
}

bool CGameProtocol::AssignLength( QByteArray &content )
{
	// insert the actual length of the content array into bytes 3 and 4 (indices 2 and 3)

	if( content.size( ) >= 4 && content.size( ) <= 65535 )
	{
		QByteArray LengthBytes;
		uchar dest[2];
		qToLittleEndian<quint16>(content.size(), dest);
		LengthBytes = QByteArray((char*)dest, 2);
		content[2] = LengthBytes[0];
		content[3] = LengthBytes[1];
		return true;
	}

	return false;
}

bool CGameProtocol::ValidateLength( QByteArray &content )
{
	// verify that bytes 3 and 4 (indices 2 and 3) of the content array describe the length

	quint16 Length;
	QByteArray LengthBytes;

	if( content.size( ) >= 4 && content.size( ) <= 65535 )
	{
		LengthBytes.push_back( content[2] );
		LengthBytes.push_back( content[3] );
		Length = qFromLittleEndian<quint16>((uchar*)LengthBytes.data());

		if( Length == content.size( ) )
			return true;
	}

	return false;
}

QByteArray CGameProtocol::DecodeStatString( QByteArray &data )
{
	quint8 Mask;
	QByteArray Result;

	for( int i = 0; i < data.size( ); i++ )
	{
		if( ( i % 8 ) == 0 )
			Mask = data[i];
		else
		{
			if( ( Mask & ( 1 << ( i % 8 ) ) ) == 0 )
				Result.push_back( data[i] - 1 );
			else
				Result.push_back( data[i] );
		}
	}

	return Result;
}

CGameInfo* CGameProtocol :: RECEIVE_W3GS_GAMEINFO ( QByteArray data )
{
	if( ValidateLength( data ) && data.size( ) >= 16 )
	{
		QDataStream stream(data);

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

		stream.skipRawData(4);
		stream.setByteOrder(QDataStream::LittleEndian);

		stream >> ProductID;
		stream >> Version;
		stream >> HostCounter;
		stream >> EntryKey;
		GameName = ExtractString(stream);
		GamePassword = ExtractString(stream);
		StatString = ExtractString(stream);
		stream >> SlotsTotal;
		stream >> MapGameType;
		stream >> Unknown2;
		stream >> SlotsOpen;
		stream >> UpTime;
		stream >> Port;

		QByteArray DecStatString = CGameProtocol::DecodeStatString( StatString );
		QDataStream ds(DecStatString);
		ds.setByteOrder(QDataStream::LittleEndian);
		ds.skipRawData(5);

		quint16 MapWidth;
		quint16 MapHeight;

		ds >> MapWidth;
		ds >> MapHeight;

		bool Reliable = true;//((MapWidth == 1984) && (MapHeight == 1984));

		return new CGameInfo(ProductID, Version, HostCounter, EntryKey, GameName, GamePassword, StatString, SlotsTotal, MapGameType, Unknown2, SlotsOpen, UpTime, Port, Reliable);
	}

    return NULL;

}

QByteArray CGameProtocol :: SEND_W3GS_DECREATEGAME( quint32 entrykey )
{
	QByteArray packet;
	QDataStream ds(&packet, QIODevice::ReadWrite);
	ds.setByteOrder(QDataStream::LittleEndian);

	ds << (quint8)W3GS_HEADER_CONSTANT;
	ds << (quint8)W3GS_DECREATEGAME;
	ds << (quint16)0;
	ds << entrykey;
	AssignLength( packet );

	return packet;
}

QByteArray CGameProtocol :: SEND_W3GS_CHAT_FROM_HOST( quint8 fromPID, QByteArray toPIDs, quint8 flag, quint32 flagExtra, QString message )
{
	QByteArray packet;
	QDataStream ds(&packet, QIODevice::ReadWrite);
	ds.setByteOrder(QDataStream::LittleEndian);

	if( !toPIDs.isEmpty( ) && !message.isEmpty( ) && message.size( ) < 255 )
	{
		ds << (quint8)W3GS_HEADER_CONSTANT;
		ds << (quint8)W3GS_CHAT_FROM_HOST;
		ds << (quint16)0;
		ds << (quint8)toPIDs.size();							// number of receivers
		ds.writeRawData(toPIDs.data(), toPIDs.length());
		ds << fromPID;
		ds << flag;

        if(flagExtra != (quint32)-1)
			ds << flagExtra;

		ds.writeRawData(message.toAscii(), message.length());
		ds << (quint8)0;
		AssignLength( packet );
	}

	// DEBUG_Print( "SENT W3GS_CHAT_FROM_HOST" );
	// DEBUG_Print( packet );
	return packet;
}

quint32 CGameInfo::NextUniqueGameID = 1;

CGameInfo::CGameInfo(quint32 nProductID, quint32 nVersion, quint32 nHostCounter, quint32 nEntryKey,
					 QString nGameName, QString nGamePassword, QByteArray nStatString, quint32 nSlotsTotal,
					 quint32 nMapGameType, quint32 nUnknown2, quint32 nSlotsOpen, quint32 nUpTime, quint16 nPort, bool nReliable) :
	ProductID(nProductID), Version(nVersion), HostCounter(nHostCounter), EntryKey(nEntryKey),
	GameName(nGameName), GamePassword(nGamePassword), StatString(nStatString), SlotsTotal(nSlotsTotal),
	MapGameType(nMapGameType), Unknown2(nUnknown2), SlotsOpen(nSlotsOpen), UpTime(nUpTime), Port(nPort), Reliable(nReliable)
{
	UniqueGameID = NextUniqueGameID++;
}

QByteArray CGameInfo::GetPacket(quint16 port)
{
	QByteArray packet;
	QDataStream ds(&packet, QIODevice::ReadWrite);
	ds.setByteOrder(QDataStream::LittleEndian);

	ds << (quint8)W3GS_HEADER_CONSTANT;
	ds << (quint8)CGameProtocol :: W3GS_GAMEINFO;
	ds << (quint16)0;
	ds << ProductID;
	ds << Version;
	ds << UniqueGameID;
	ds << UniqueGameID;
	ds.writeRawData(GameName.toUtf8(), GameName.length());
	ds << (quint8)0;
	ds.writeRawData(GamePassword.toUtf8(), GamePassword.length());
	ds << (quint8)0;
	ds.writeRawData(StatString.data(), StatString.length());
	ds << SlotsTotal;
	ds << MapGameType;
	ds << Unknown2;
	ds << SlotsOpen;
	ds << UpTime;
	ds << port;

	QByteArray LengthBytes;
	uchar dest[2];
	qToLittleEndian<quint16>(packet.size(), dest);
	LengthBytes = QByteArray((char*)dest, 2);
	packet[2] = LengthBytes[0];
	packet[3] = LengthBytes[1];

	return packet;
}
