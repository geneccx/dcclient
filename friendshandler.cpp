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

*/

#include "friendshandler.h"

FriendsHandler::FriendsHandler(QString nFriendName, QWidget *parent) :
    QObject(parent), m_FriendName(nFriendName)
{
	m_Status = false;

	if(parent)
	{
		connect(this, SIGNAL(showMessage(QString, MessageType)), parent, SLOT(showTextCurrentTab(QString, MessageType)));
		connect(this, SIGNAL(requestGame(QString)), parent, SIGNAL(requestGame(QString)));
	}

/*	if(m_Buffer)
	{
		connect(m_Buffer, SIGNAL(joined(const QString)), this, SLOT(joined(const QString)));
		connect(m_Buffer, SIGNAL(parted(const QString, const QString)), this, SLOT(parted(const QString, const QString)));
		connect(m_Buffer, SIGNAL(quit(const QString, const QString)), this, SLOT(parted(const QString, const QString)));

		connect(m_Buffer, SIGNAL(nickChanged(const QString, const QString)), this, SLOT(nickChanged(const QString, const QString)));
		connect(m_Buffer, SIGNAL(messageReceived(const QString, const QString, Irc::Buffer::MessageFlags)), this, SLOT(messageReceived(const QString, const QString, Irc::Buffer::MessageFlags)));
		connect(m_Buffer, SIGNAL(noticeReceived(const QString, const QString, Irc::Buffer::MessageFlags)), this, SLOT(noticeReceived(const QString, const QString, Irc::Buffer::MessageFlags)));
	}
*/
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
		QString timestamp = QDateTime::currentDateTime().toString("hh:mm");

		emit showMessage(tr("[%2] Your friend %1 is now online.").arg(user).arg(timestamp), Friends);
	}
}

void FriendsHandler::parted(const QString user, const QString)
{
	if(user.toLower() == m_FriendName.toLower())
	{
		m_Status = false;
		QString timestamp = QDateTime::currentDateTime().toString("hh:mm");

		emit showMessage(tr("[%2] Your friend %1 is now offline.").arg(user).arg(timestamp), Friends);
	}
}

void FriendsHandler::messageReceived(const QString &origin, const QString &message)
{
	if(origin.toLower() == m_FriendName.toLower())
	{
		QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
		emit showMessage(QString("[%3]&lt;%1&gt; %2").arg(origin).arg(message).arg(timestamp), Friends);
	}
}

void FriendsHandler::noticeReceived(const QString &origin, const QString &message)
{
	if(origin.toLower() == m_FriendName.toLower())
	{
		if(message.startsWith("xdcc://"))
		{
			int idx = message.indexOf(";");

			QString IP = message.mid(7).left(idx-7);
			QString gameName = message.mid(idx+1);

			QString timestamp = QDateTime::currentDateTime().toString("hh:mm");

			emit showMessage(tr("[%4] Your friend %1 has joined the game <a href=xdcc://%2>%3</a>").arg(origin).arg(IP).arg(gameName).arg(timestamp), Friends);

			QSettings settings("DotaCash", "DCClient X");
			if(settings.value("FriendFollow", true).toBool())
				emit requestGame(IP);
		}
	}
}
