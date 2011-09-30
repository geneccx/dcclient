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

#include "ui_xdcc_main.h"
#include "channelhandler.h"
#include "friendshandler.h"

#include <qstringlist.h>
#include <IrcUtil>

IrcHandler::IrcHandler(QWidget* parent)
	: QTabWidget(parent), irc(0)
{
	connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(myCloseTab(int)));

	QString friendsList = QSettings("DotaCash", "DCClient X", this).value("Friends").toString();

	if(!friendsList.isEmpty())
		m_Friends = friendsList.toLower().split(";");
}

void IrcHandler::connectToIrc(QString name)
{
	m_Username = name;

	irc = new IrcSession(this);

	connect(irc, SIGNAL(connected()), this, SLOT(connected()));
	connect(irc, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(irc, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(messageReceived(IrcMessage*)));

	irc->setHost("irc.dotacash.com");
    irc->setPort(6667);
	irc->setNickName(m_Username);
	irc->setUserName("dcclient");
	irc->setRealName(m_Username);
	irc->open();
}

void IrcHandler::myCloseTab(int idx)
{
	QString channel = this->tabText(idx);
	if(channel.startsWith("##") || channel == "#dcchat" || channel == "#dotacash")
		return;

	else
	{
		if(channel.at(0) == '#')
			irc->sendCommand(IrcCommand::createPart(channel));

		removeTabName(channel);
	}
}

void IrcHandler::connected()
{
	emit showMessage(tr("Connected to %1").arg("irc.dotacash.com"), 3000);

	irc->sendCommand(IrcCommand::createJoin("#dcchat"));
	irc->sendCommand(IrcCommand::createJoin("#dotacash"));
	irc->sendCommand(IrcCommand::createJoin(QString("##%1").arg(m_Username)));
	
	for(int i = 0; i < m_Friends.size(); ++i)
		irc->sendCommand(IrcCommand::createJoin(QString("##%1").arg(m_Friends.at(i).toLower())));
}

void IrcHandler::disconnected()
{
	for(int i = 0; i < m_ChannelMap.size(); ++i)
		delete m_ChannelMap.values().at(i);

	m_ChannelMap.clear();

	emit showMessage(tr("Disconnected from %1, reconnecting...").arg("irc.dotacash.com"), 3000);
}

void IrcHandler::handleChat(QString& origin, QString& Message)
{
	if(!irc)
		return;

	if(Message.isEmpty())
		return;

	ChannelHandler* channel = m_ChannelMap[origin.toLower()];

	if(Message.startsWith('/'))
	{
		QStringList Payload = Message.split(' ');
		QString Command;

		Command = Payload.at(0);
		Payload.pop_front();

		if((Command == "/join" || Command == "/j") && !Payload.isEmpty())
		{
			QString JoinChan = Payload.at(0);

			if(JoinChan.startsWith("##"))
				return;

			irc->sendCommand(IrcCommand::createJoin(JoinChan));
		}

		else if(Command == "/part" || Command == "/p")
		{
			if(Payload.isEmpty())
			{
				if(origin.startsWith("##") || origin == "#dcchat" || origin == "#dotacash")
					return;

				irc->sendCommand(IrcCommand::createPart(origin));
				removeTabName(origin.toLower());
			}
			else
			{
				if(Payload.at(0).startsWith("##") || Payload.at(0) == "#dcchat" || Payload.at(0) == "#dotacash")
					return;

				irc->sendCommand(IrcCommand::createPart(Payload.at(0)));
			}
		}

		else if(Command == "/nick" && !Payload.empty())
		{
			irc->sendCommand(IrcCommand::createNick(Payload.at(0)));
			irc->setNickName(Payload.at(0));
		}

		else if((Command == "/whisper" || Command == "/w") && Payload.size() >= 2)
		{
			QString to = Payload.at(0);
			Payload.pop_front();

			QString& txt = Payload.join(" ");

			ChannelHandler* handler = m_ChannelMap[to.toLower()];

			if(!handler)
			{
				handler = new ChannelHandler(false, this);
				m_ChannelMap[to.toLower()] = handler;

				this->addTab(handler->GetTab(), to);
				this->setCurrentIndex(this->count() - 1);
			}

			handler->showText(QString("&lt;%1&gt; %2").arg(irc->nickName()).arg(txt));
			irc->sendCommand(IrcCommand::createMessage(to, txt));
		}

		else if((Command == "/f" || Command == "/friends") && !Payload.isEmpty())
		{
			if(Payload.at(0) == "l" || Payload.at(0) == "list")
			{
				int count = m_FriendsMap.size();

				if(!count)
					showTextCurrentTab(tr("You currently have no friends :("), Err);

				else
				{
					showTextCurrentTab(tr("Your friends are:"), Sys);

					for(int i = 0; i < count; ++i)
					{
						QString friendName = m_FriendsMap.keys().at(i);
						FriendsHandler* myFriend = m_FriendsMap.values().at(i);

						if(myFriend)
						{
							QString status = myFriend->GetStatus() ? tr("online") : tr("offline");

							showTextCurrentTab(QString("%1: %2 - %3").arg(i+1).arg(friendName).arg(status), Sys);
						}
					}
				}
			}

			else if(Payload.at(0) == "a")
			{
				if(Payload.length() == 1)
					showTextCurrentTab(tr("You need to supply the name of the friend you wish to add to your list."), Err);
				else
				{
					QString friendName = Payload.at(1).toLower();

					if(m_Friends.contains(friendName, Qt::CaseInsensitive))
						showTextCurrentTab(tr("%1 is already in your friends list.").arg(friendName), Err);
					else if(friendName.toLower() == m_Username.toLower())
						showTextCurrentTab(tr("You cannot add yourself your friends list."), Err);
					else
					{
						m_Friends << friendName;
						irc->sendCommand(IrcCommand::createJoin(QString("##%1").arg(friendName)));

						QSettings("DotaCash", "DCClient X", this).setValue("Friends", m_Friends.join(";"));
						showTextCurrentTab(tr("Added %1 to your friends list.").arg(friendName), Sys);
					}
				}
			}

			else if(Payload.at(0) == "r")
			{
				if(Payload.length() == 1)
					showTextCurrentTab(tr("You need to supply the name of the friend you wish to remove from your list."), Err);
				else
				{
					QString friendName = Payload.at(1).toLower();

					if(!m_Friends.contains(friendName, Qt::CaseInsensitive))
						showTextCurrentTab(tr("%1 is not in your friends list.").arg(friendName), Err);
					else
					{
						int idx = m_Friends.indexOf(friendName);
						m_Friends.removeAt(idx);

						irc->sendCommand(IrcCommand::createPart(QString("##%1").arg(friendName)));

						delete m_FriendsMap[friendName.toLower()];
						m_FriendsMap.remove(friendName.toLower());

						QSettings("DotaCash", "DCClient X", this).setValue("Friends", m_Friends.join(";"));
						showTextCurrentTab(tr("Removed %1 from your friends list.").arg(friendName), Sys);
					}
				}
			}

			else if(Payload.at(0) == "m" || Payload.at(0) == "msg")
			{
				if(Payload.length() == 1)
					showTextCurrentTab(tr("What do you want to say?"), Err);
				else
				{
					Payload.pop_front();
					QString msg = Payload.join(" ");

					irc->sendCommand(IrcCommand::createMessage(QString("##%1").arg(m_Username.toLower()), msg));
					showTextCurrentTab(tr("You whisper to your friends: %1").arg(msg), Sys);
				}
			}
		}

		else
		{
			showTextCurrentTab(tr("Invalid command."), Err);
		}
	}
	else
	{
		irc->sendCommand(IrcCommand::createMessage(origin.toLower(), Message));
		showTextCurrentTab(QString("&lt;%1&gt; %2").arg(irc->nickName()).arg(Message));
	}
}

void IrcHandler::joinedChannel(IrcPrefix origin, IrcJoinMessage* joinMsg)
{
	if(!origin.isValid())
		return;

	QString& user = origin.name();
	QString& channel = joinMsg->channel();
	QString nameLower = channel.toLower();

	if(user.toLower() == irc->nickName().toLower())
	{
		if(channel.startsWith("##"))
		{
			if(nameLower != ("##" + m_Username.toLower()))
			{
				FriendsHandler* handler = m_FriendsMap[nameLower.mid(2)];
			
				if(!handler)
				{
					handler = new FriendsHandler(channel.mid(2), this);
					m_FriendsMap[nameLower.mid(2)] = handler;
				}
			}
		}
		else
		{
			ChannelHandler* handler = m_ChannelMap[nameLower];

			if(!handler)
			{
				handler = new ChannelHandler(channel.startsWith('#'), this);
				m_ChannelMap[nameLower] = handler;

				this->addTab(handler->GetTab(), channel);

				int idx = this->count() - 1;

				if(nameLower == "#dotacash" || nameLower == "#dcchat")
					this->tabBar()->setTabButton(idx, QTabBar::RightSide, 0);

				this->setCurrentIndex(idx);
			}
		}
	}
	else
	{
		if(channel.startsWith("##"))
		{
			if(nameLower != ("##" + m_Username.toLower()))
			{
				FriendsHandler* handler = m_FriendsMap[nameLower.mid(2)];
			
				if(handler)
					handler->joined(user);
			}
		}
		else
		{
			ChannelHandler* handler = m_ChannelMap[nameLower];

			if(handler)
				handler->joined(user);
		}

	}
}

void IrcHandler::partedChannel(IrcPrefix origin, IrcPartMessage* partMsg)
{
	if(!origin.isValid())
		return;

	QString& user = origin.name();
	QString& channel = partMsg->channel();

	if(user.toLower() == irc->nickName().toLower())
		removeTabName(channel.toLower());
	else
	{
		if(channel.startsWith("##"))
		{
			if(channel.toLower() != ("##" + m_Username.toLower()))
			{
				FriendsHandler* handler = m_FriendsMap[channel.toLower().mid(2)];
			
				if(handler)
					handler->parted(user, partMsg->reason());
			}
		}
		else
		{
			ChannelHandler* handler = m_ChannelMap[channel.toLower()];

			if(handler)
				handler->parted(user, partMsg->reason());
		}
	}
}

void IrcHandler::quitMessage(IrcPrefix origin, IrcQuitMessage* quitMsg)
{
	if(!origin.isValid())
		return;

	QString& user = origin.name();

	for(QMap<QString, ChannelHandler*>::iterator i = m_ChannelMap.constBegin(); i != m_ChannelMap.constEnd(); ++i)
	{
		if(!i.key().isEmpty() && (i.value() != NULL))
			i.value()->parted(user, quitMsg->reason());
	}

	for(QMap<QString, FriendsHandler*>::iterator i = m_FriendsMap.constBegin(); i != m_FriendsMap.constEnd(); ++i)
	{
		if(!i.key().isEmpty() && (i.value() != NULL))
			i.value()->parted(user, quitMsg->reason());
	}
}

void IrcHandler::privateMessage(IrcPrefix origin, IrcPrivateMessage* privMsg)
{
	if(!origin.isValid())
		return;

	QString& user = origin.name();
	QString& target = privMsg->target();
	QString& message = privMsg->message();

	QString txt = QString("<%1> %2").arg(user).arg(message);

	if(target.startsWith("##"))
	{
		if(target.toLower() != ("##" + m_Username.toLower()))
		{
			FriendsHandler* handler = m_FriendsMap[target.toLower().mid(2)];
			
			if(handler)
				handler->messageReceived(user, message);
		}
	}
	else
	{
		if(target.toLower() != irc->nickName().toLower())
		{
			ChannelHandler* handler = m_ChannelMap[target.toLower()];

			if(!handler)
			{
				handler = new ChannelHandler(true, this);
				m_ChannelMap[target.toLower()] = handler;

				this->addTab(handler->GetTab(), target);
				this->setCurrentIndex(this->count() - 1);
			}

			handler->showText(txt);
		}
		else
		{
			ChannelHandler* handler = m_ChannelMap[user.toLower()];

			if(!handler)
			{
				handler = new ChannelHandler(false, this);
				m_ChannelMap[user.toLower()] = handler;

				this->addTab(handler->GetTab(), user);
				this->setCurrentIndex(this->count() - 1);
			}

			handler->showText(txt);
		}

	}
}

void IrcHandler::noticeMessage(IrcPrefix origin, IrcNoticeMessage* noticeMsg)
{
	if(!origin.isValid())
		return;

	QString& user = origin.name();
	QString& target = noticeMsg->target();
	QString& message = noticeMsg->message();

	if(target.startsWith("##"))
	{
		if(target.toLower() != ("##" + m_Username.toLower()))
		{
			FriendsHandler* handler = m_FriendsMap[target.toLower().mid(2)];
			
			if(handler)
				handler->noticeReceived(user, message);
		}
	}
	else if(target.toLower() != irc->nickName().toLower())
	{
		ChannelHandler* handler = m_ChannelMap[target.toLower()];

		if(!handler)
		{
			handler = new ChannelHandler(target.startsWith('#'), this);
			m_ChannelMap[target.toLower()] = handler;

			this->addTab(handler->GetTab(), target);
			this->setCurrentIndex(this->count() - 1);
		}

		handler->noticeReceived(user, message);
	}
	else
	{
		QString txt = QString("<span class = \"notice\">-%1- %2</span>").arg(user).arg(IrcUtil::messageToHtml(message));
		showTextCurrentTab(txt);
	}
}

void IrcHandler::nickMessage(IrcPrefix origin, IrcNickMessage* nickMsg)
{
	if(!origin.isValid())
		return;
	
	QString& user = origin.name();

	if(user == irc->nickName())
		irc->setNickName(nickMsg->nick());

	for(QMap<QString, ChannelHandler*>::iterator i = m_ChannelMap.constBegin(); i != m_ChannelMap.constEnd(); ++i)
	{
		if(!i.key().isEmpty() && (i.value() != NULL))
			i.value()->nickChanged(user, nickMsg->nick());
	}

	for(QMap<QString, FriendsHandler*>::iterator i = m_FriendsMap.constBegin(); i != m_FriendsMap.constEnd(); ++i)
	{
		if(!i.key().isEmpty() && (i.value() != NULL))
			i.value()->nickChanged(user, nickMsg->nick());
	}
}

void IrcHandler::numericMessage(IrcNumericMessage* numMsg)
{	
	int code = numMsg->code();
	IrcMessage* msg = numMsg;

	switch(code)
	{
	case Irc::RPL_NAMREPLY:
		QString& channel = msg->parameters().at(2).toLower();
		QStringList names = msg->parameters().at(3).split(' ');

		if(channel.startsWith("##"))
		{
			if(channel.mid(2) != m_Username.toLower())
			{
				FriendsHandler* target = m_FriendsMap[channel.mid(2)];

				if(target)
				{
					if(names.contains(channel.mid(2), Qt::CaseInsensitive))
						target->SetStatus(true);
				}
			}
		}
		else
		{
			ChannelHandler* target = m_ChannelMap[channel];

			if(target)
				target->UpdateNames(names);
		}

		break;
	}
}

void IrcHandler::messageReceived(IrcMessage *msg)
{
	switch(msg->type())
	{
	case IrcMessage::Join:
		joinedChannel(msg->prefix(), dynamic_cast<IrcJoinMessage*>(msg));
		break;
	case IrcMessage::Part:
		partedChannel(msg->prefix(), dynamic_cast<IrcPartMessage*>(msg));
		break;
	case IrcMessage::Quit:
		quitMessage(msg->prefix(), dynamic_cast<IrcQuitMessage*>(msg));
		break;
	case IrcMessage::Private:
		privateMessage(msg->prefix(), dynamic_cast<IrcPrivateMessage*>(msg));
		break;
	case IrcMessage::Notice:
		noticeMessage(msg->prefix(), dynamic_cast<IrcNoticeMessage*>(msg));
		break;
	case IrcMessage::Nick:
		nickMessage(msg->prefix(), dynamic_cast<IrcNickMessage*>(msg));
		break;
	case IrcMessage::Numeric:
		numericMessage(dynamic_cast<IrcNumericMessage*>(msg));
		break;
	}
}

void IrcHandler::removeTabName(QString name)
{
	ChannelHandler* chan = m_ChannelMap[name.toLower()];

	if(chan)
	{
		for(int i = 0; i < this->count(); i++)
		{
			if(this->tabText(i).toLower() == name.toLower())
			{
				this->removeTab(i);
				break;
			}
		}

		delete chan;

		m_ChannelMap.remove(name.toLower());
	}
}

void IrcHandler::showTextCurrentTab(QString message, MessageType msgType)
{
	QString channel = this->tabText(this->currentIndex());
	ChannelHandler* handler = m_ChannelMap[channel.toLower()];

	QString spanClass;

	switch(msgType)
	{
	case Err:
		spanClass = "err";
		break;
	case Friends:
		spanClass = "fmsg";
		break;
	case Sys:
		spanClass = "sysmsg";
		break;
	case Normal:
	default:
		spanClass = "msg";
		break;
	}

	if(handler)
		handler->showText(QString("<span class=\"%1\">%2</span>").arg(spanClass).arg(message));
}

void IrcHandler::reloadSkin()
{
	for(int i = 0; i < m_ChannelMap.count(); ++i)
	{
		ChannelHandler* handler = m_ChannelMap.values().at(i);

		if(handler)
			handler->reloadSkin();
	}
}

void IrcHandler::joinedGame(QString ip, QString gameName)
{
	irc->sendCommand(IrcCommand::createNotice(QString("##%1").arg(m_Username.toLower()),
					 QString("xdcc://%1;%2").arg(ip).arg(gameName)));
}

void IrcHandler::handleUrl(QUrl url)
{
	QString data = url.toString();

	if(data.startsWith("xdcc://"))
	{
		emit requestGame(data.mid(7));
	}
}
