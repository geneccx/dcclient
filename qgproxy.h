/*

   Copyright 2011 Gene Chen

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   With code derived from gproxy: http://code.google.com/p/gproxyplusplus/

*/

#ifndef QGPROXY_H
#define QGPROXY_H

#include "xdcc.h"

#include <QQueue>
#include <QTcpSocket>
#include <QTcpServer>
#include <QUdpSocket>

class CGameInfo;
class CGameProtocol;
class CCommandPacket;
class CGPSProtocol;

class CGProxy : public QObject
{
	Q_OBJECT

public:
	CGProxy(QWidget* parent=0);
	~CGProxy();

	void requestGame(QString ip);
	const QVector<CGameInfo*> getGames() { return games; }

public slots:
	void update();
	void readPendingDatagrams();
	void newConnection();
	void readLocalPackets();
	void readServerPackets();

	void localDisconnected();
	void remoteDisconnected();

signals:
	void joinedGame(QString, QString);

private:
	QUdpSocket* m_RequesterSocket;

	QTcpServer* m_LocalServer;

	QTcpSocket* m_RemoteSocket;
	QTcpSocket* m_LocalSocket;

	CGameProtocol* m_GameProtocol;
	QVector<CGameInfo*> games;

	CGPSProtocol *m_GPSProtocol;

	QTimer* timer;

	QString m_RemoteServerIP;

	QByteArray m_RemoteBytes;
	QByteArray m_LocalBytes;

	QQueue<CCommandPacket *> m_LocalPackets;
	QQueue<CCommandPacket *> m_RemotePackets;
	QQueue<CCommandPacket *> m_PacketBuffer;

	QVector<quint8> m_Laggers;
	quint32 m_TotalPacketsReceivedFromLocal;
	quint32 m_TotalPacketsReceivedFromRemote;
	quint32 m_LastAckTime;
	quint32 m_LastActionTime;
	quint32 m_LastConnectionAttemptTime;
	quint32 m_ReconnectKey;
	quint32 m_LastBroadcastTime;

	quint16 m_ListenPort;
	quint16 m_ReconnectPort;

	quint8 m_PID;
	quint8 m_ChatPID;
	quint8 m_NumEmptyActions;
	quint8 m_NumEmptyActionsUsed;

	bool m_ActionReceived;
	bool m_GameIsReliable;
	bool m_GameStarted;
	bool m_Synchronized;
	bool m_LeaveGameSent;

	void parsePacket(QString, QByteArray);

	void processLocalPackets();
	void processServerPackets();

	void SendLocalChat( QString message );
	void SendEmptyAction( );
};

#endif
