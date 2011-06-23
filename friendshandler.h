#ifndef FRIENDSHANDLER_H
#define FRIENDSHANDLER_H

#include "xdcc.h"
#include <QObject>
#include <QTextBrowser>
#include <QListWidget>

#define IRC_STATIC
#include <ircclient-qt/Irc>
#include <ircclient-qt/IrcBuffer>

class FriendsHandler : public QObject
{
	Q_OBJECT

public:
	FriendsHandler(Irc::Buffer* nBuffer, QString nFriendName, QWidget *parent=0);

	Irc::Buffer* GetBuffer() { return m_Buffer; }

	bool GetStatus() { return m_Status; }

	void SetStatus(bool nStatus) { m_Status = nStatus; }

public slots:
	void joined(const QString);
	void parted(const QString, const QString);

	void nickChanged(const QString, const QString);
	void messageReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags=Irc::Buffer::NoFlags);
	void noticeReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags=Irc::Buffer::NoFlags);

signals:
	void showMessage(QString, MessageType msgType = Normal);
	void requestGame(QString);

private:
	Irc::Buffer* m_Buffer;
	QString m_FriendName;

	bool m_Status;
};

#endif
