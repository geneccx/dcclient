#include "friendshandler.h"

FriendsHandler::FriendsHandler(Irc::Buffer* nBuffer, QString nFriendName, QWidget *parent) :
	m_Buffer(nBuffer), m_FriendName(nFriendName), QObject(parent)
{
	m_Status = false;

	if(parent)
	{
		connect(this, SIGNAL(showMessage(QString, MessageType)), parent, SLOT(showTextCurrentTab(QString, MessageType)));
		connect(this, SIGNAL(requestGame(QString)), parent, SIGNAL(requestGame(QString)));
	}

	if(m_Buffer)
	{
		connect(m_Buffer, SIGNAL(joined(const QString)), this, SLOT(joined(const QString)));
		connect(m_Buffer, SIGNAL(parted(const QString, const QString)), this, SLOT(parted(const QString, const QString)));
		connect(m_Buffer, SIGNAL(quit(const QString, const QString)), this, SLOT(parted(const QString, const QString)));

		connect(m_Buffer, SIGNAL(nickChanged(const QString, const QString)), this, SLOT(nickChanged(const QString, const QString)));
		connect(m_Buffer, SIGNAL(messageReceived(const QString, const QString, Irc::Buffer::MessageFlags)), this, SLOT(messageReceived(const QString, const QString, Irc::Buffer::MessageFlags)));
		connect(m_Buffer, SIGNAL(noticeReceived(const QString, const QString, Irc::Buffer::MessageFlags)), this, SLOT(noticeReceived(const QString, const QString, Irc::Buffer::MessageFlags)));
	}
}

void FriendsHandler::nickChanged(const QString user, const QString nick)
{
	if(user.toLower() == m_FriendName.toLower())
		m_FriendName = nick;
}

void FriendsHandler::joined(const QString user)
{
	if(user.toLower() == m_FriendName.toLower())
	{
		m_Status = true;

		emit showMessage(tr("Your friend %1 is now online.").arg(user), Friends);
	}
}

void FriendsHandler::parted(const QString user, const QString reason)
{
	if(user.toLower() == m_FriendName.toLower())
	{
		m_Status = false;

		emit showMessage(tr("Your friend %1 is now offline.").arg(user), Friends);
	}
}

void FriendsHandler::messageReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags)
{
	if(origin.toLower() == m_FriendName.toLower())
	{
		emit showMessage(QString("&lt;%1&gt; %2").arg(origin).arg(message), Friends);
	}
}

void FriendsHandler::noticeReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags)
{
	if(origin.toLower() == m_FriendName.toLower())
	{
		if(message.startsWith("xdcc://"))
		{
			int idx = message.indexOf(";");

			QString IP = message.mid(7).left(idx-7);
			QString gameName = message.mid(idx+1);
			
			emit showMessage(tr("Your friend %1 has joined the game <a href=xdcc://%2>%3</a>").arg(origin).arg(IP).arg(gameName), Friends);

			QSettings settings("DotaCash", "DCClient X");
			if(settings.value("FriendFollow", true).toBool())
				emit requestGame(IP);
		}
	}
}