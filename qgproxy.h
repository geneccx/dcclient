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

private:
	QUdpSocket* m_UDPSocket;
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