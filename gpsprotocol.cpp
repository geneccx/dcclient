/*

   Copyright 2010 Trevor Hogan

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

#include "gpsprotocol.h"
#include <QtEndian>

//
// CGPSProtocol
//

CGPSProtocol :: CGPSProtocol( )
{

}

CGPSProtocol :: ~CGPSProtocol( )
{

}

///////////////////////
// RECEIVE FUNCTIONS //
///////////////////////

////////////////////
// SEND FUNCTIONS //
////////////////////

QByteArray CGPSProtocol :: SEND_GPSC_INIT( quint32 version )
{
	QByteArray packet;
	packet.push_back( GPS_HEADER_CONSTANT );
	packet.push_back( GPS_INIT );
	packet.push_back( (char)0 );
	packet.push_back( (char)0 );
	packet.append( (char*)&version, 4 );
	AssignLength( packet );

	return packet;
}

QByteArray CGPSProtocol :: SEND_GPSC_RECONNECT( quint8 PID, quint32 reconnectKey, quint32 lastPacket )
{
	QByteArray packet;
	packet.push_back( GPS_HEADER_CONSTANT );
	packet.push_back( GPS_RECONNECT );
	packet.push_back( (char)0 );
	packet.push_back( (char)0 );
	packet.push_back( PID );
	packet.append( (char*)&reconnectKey, 4 );
	packet.append( (char*)&lastPacket, 4 );
	AssignLength( packet );
	return packet;
}

QByteArray CGPSProtocol :: SEND_GPSC_ACK( quint32 lastPacket )
{
	QByteArray packet;
	packet.push_back( GPS_HEADER_CONSTANT );
	packet.push_back( GPS_ACK );
	packet.push_back( (char)0 );
	packet.push_back( (char)0 );
	packet.append( (char*)&lastPacket, 4 );
	AssignLength( packet );
	return packet;
}

QByteArray CGPSProtocol :: SEND_GPSS_INIT( quint16 reconnectPort, quint8 PID, quint32 reconnectKey, quint8 numEmptyActions )
{
	QByteArray packet;
	packet.push_back( GPS_HEADER_CONSTANT );
	packet.push_back( GPS_INIT );
	packet.push_back( (char)0 );
	packet.push_back( (char)0 );
	packet.append( (char*)&reconnectPort, 2 );
	packet.push_back( PID );
	packet.append( (char*)&reconnectKey, 4 );
	packet.push_back( numEmptyActions );
	AssignLength( packet );
	return packet;
}

QByteArray CGPSProtocol :: SEND_GPSS_RECONNECT( quint32 lastPacket )
{
	QByteArray packet;
	packet.push_back( GPS_HEADER_CONSTANT );
	packet.push_back( GPS_RECONNECT );
	packet.push_back( (char)0 );
	packet.push_back( (char)0 );
	packet.append( (char*)&lastPacket, 4 );
	AssignLength( packet );
	return packet;
}

QByteArray CGPSProtocol :: SEND_GPSS_ACK( quint32 lastPacket )
{
	QByteArray packet;
	packet.push_back( GPS_HEADER_CONSTANT );
	packet.push_back( GPS_ACK );
	packet.push_back( (char)0 );
	packet.push_back( (char)0 );
	packet.append( (char*)&lastPacket, 4 );
	AssignLength( packet );
	return packet;
}

QByteArray CGPSProtocol :: SEND_GPSS_REJECT( quint32 reason )
{
	QByteArray packet;
	packet.push_back( GPS_HEADER_CONSTANT );
	packet.push_back( GPS_REJECT );
	packet.push_back( (char)0 );
	packet.push_back( (char)0 );
	packet.append( (char*)&reason, 4 );
	AssignLength( packet );
	return packet;
}

/////////////////////
// OTHER FUNCTIONS //
/////////////////////

bool CGPSProtocol :: AssignLength( QByteArray &content )
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

bool CGPSProtocol :: ValidateLength( QByteArray &content )
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
