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

#ifndef GPSPROTOCOL_H
#define GPSPROTOCOL_H

#include <QByteArray>

//
// CGameProtocol
//

#define GPS_HEADER_CONSTANT	248

#define REJECTGPS_INVALID	1
#define REJECTGPS_NOTFOUND	2

class CGPSProtocol
{
public:
	enum Protocol {
		GPS_INIT	= 1,
		GPS_RECONNECT	= 2,
		GPS_ACK		= 3,
		GPS_REJECT	= 4
	};

	CGPSProtocol( );
	~CGPSProtocol( );

	// receive functions

	// send functions

	QByteArray SEND_GPSC_INIT( quint32 version );
	QByteArray SEND_GPSC_RECONNECT( quint8 PID, quint32 reconnectKey, quint32 lastPacket );
	QByteArray SEND_GPSC_ACK( quint32 lastPacket );

	QByteArray SEND_GPSS_INIT( quint16 reconnectPort, quint8 PID, quint32 reconnectKey, quint8 numEmptyActions );
	QByteArray SEND_GPSS_RECONNECT( quint32 lastPacket );
	QByteArray SEND_GPSS_ACK( quint32 lastPacket );
	QByteArray SEND_GPSS_REJECT( quint32 reason );

	// other functions

private:
	bool AssignLength( QByteArray &content );
	bool ValidateLength( QByteArray &content );
};

#endif
