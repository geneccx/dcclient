#include "ui_xdcc_main.h"
#include "channelhandler.h"
#include "friendshandler.h"

IrcHandler::IrcHandler(QWidget* parent) : QTabWidget(parent), irc(0), m_Buffer(0)
{
	connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(myCloseTab(int)));

	QString friendsList = QSettings("DotaCash", "DCClient X", this).value("Friends").toString();

	if(!friendsList.isEmpty())
		m_Friends = friendsList.toLower().split(";");
}

void IrcHandler::connectToIrc(QString name)
{
	m_Username = name;

	irc = new Irc::Session(this);

	connect(irc, SIGNAL(connected()), this, SLOT(irc_connected()));
	connect(irc, SIGNAL(disconnected()), this, SLOT(irc_disconnected()));
	connect(irc, SIGNAL(bufferAdded (Irc::Buffer*)), this, SLOT(irc_buffer_added(Irc::Buffer*)));
	connect(irc, SIGNAL(bufferRemoved (Irc::Buffer*)), this, SLOT(irc_buffer_removed(Irc::Buffer*)));

	irc->setNick(m_Username);
	irc->setIdent("dcclient");
	irc->setRealName(m_Username);
	irc->addAutoJoinChannel("#dcchat");
	irc->addAutoJoinChannel("#dotacash");
	irc->addAutoJoinChannel(QString("##%1").arg(m_Username));

	for(int i = 0; i < m_Friends.size(); ++i)
		irc->addAutoJoinChannel(QString("##%1").arg(m_Friends.at(i)));

	irc->setAutoReconnectDelay(60);
	irc->connectToServer("irc.dotacash.com", 6667);
}

void IrcHandler::myCloseTab(int idx)
{
	QString channel = this->tabText(idx);
	if(channel.startsWith("##") || channel == "#dcchat" || channel == "#dotacash")
		return;

	else
	{
		if(channel.at(0) == '#')
			irc->part(channel);
		else
			removeTabName(channel);
	}
}

void IrcHandler::irc_connected()
{
	emit showMessage(tr("Connected to %1").arg("irc.dotacash.com"), 3000);
}

void IrcHandler::irc_disconnected()
{
	for(int i = 0; i < irc->buffers().size(); ++i)
	{
		irc_buffer_removed(irc->buffers().at(i));
		irc->buffers().at(i);
	}	

	for(int i = 0; i < m_ChannelMap.size(); ++i)
	{
		delete m_ChannelMap.values().at(i);
	}

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

			irc->join(JoinChan);
		}

		else if(Command == "/part" || Command == "/p")
		{
			if(Payload.isEmpty())
			{
				if(origin.startsWith("##") || origin == "#dcchat" || origin == "#dotacash")
					return;

				irc->part(origin);
			}
			else
			{
				if(Payload.at(0).startsWith("##") || Payload.at(0) == "#dcchat" || Payload.at(0) == "#dotacash")
					return;

				irc->part(Payload.at(0));
			}
		}

		else if(Command == "/nick" && !Payload.empty())
		{
			irc->setNick(Payload.at(0));
		}

		else if((Command == "/whisper" || Command == "/w") && Payload.size() >= 2)
		{
			QString to = Payload.at(0);
			Payload.pop_front();

			irc->message(to, Payload.join(" "));
		}

		else if((Command == "/f" || Command == "/friends") && !Payload.isEmpty())
		{
			if(Payload.at(0) == "l" || Payload.at(0) == "list")
			{
				int count = m_FriendsMap.size();

				if(!count)
					showTextCurrentTab(tr("You currently have no friends :("));

				else
				{
					showTextCurrentTab(tr("Your friends are:"));

					for(int i = 0; i < count; ++i)
					{
						QString friendName = m_FriendsMap.keys().at(i);
						FriendsHandler* myFriend = m_FriendsMap.values().at(i);

						if(myFriend)
						{
							QString status = myFriend->GetStatus() ? tr("online") : tr("offline");

							showTextCurrentTab(QString("%1: %2 - %3").arg(i+1).arg(friendName).arg(status));
						}
					}
				}
			}

			else if(Payload.at(0) == "a")
			{
				if(Payload.length() == 1)
					showTextCurrentTab(tr("You need to supply the name of the friend you wish to add to your list."));
				else
				{
					QString friendName = Payload.at(1);

					if(m_Friends.contains(friendName, Qt::CaseInsensitive))
						showTextCurrentTab(tr("%1 is already in your friends list.").arg(friendName));
					else if(friendName.toLower() == m_Username.toLower())
						showTextCurrentTab(tr("You cannot add yourself your friends list."));
					else
					{
						m_Friends << friendName;
						irc->join(QString("##%1").arg(friendName));

						QSettings("DotaCash", "DCClient X", this).setValue("Friends", m_Friends.join(";"));
						showTextCurrentTab(tr("Added %1 to your friends list.").arg(friendName));
					}
				}
			}

			else if(Payload.at(0) == "r")
			{
				if(Payload.length() == 1)
					showTextCurrentTab(tr("You need to supply the name of the friend you wish to remove from your list."));
				else
				{
					QString friendName = Payload.at(1);

					if(!m_Friends.contains(friendName, Qt::CaseInsensitive))
						showTextCurrentTab(tr("%1 is not in your friends list.").arg(friendName));
					else
					{
						int idx = m_Friends.indexOf(friendName);
						m_Friends.removeAt(idx);

						irc->part(QString("##%1").arg(friendName));

						delete m_FriendsMap[friendName.toLower()];
						m_FriendsMap.remove(friendName.toLower());

						QSettings("DotaCash", "DCClient X", this).setValue("Friends", m_Friends.join(";"));
						showTextCurrentTab(tr("Removed %1 from your friends list.").arg(friendName));
					}
				}
			}

			else if(Payload.at(0) == "m" || Payload.at(0) == "msg")
			{
				if(Payload.length() == 1)
					showTextCurrentTab(tr("What do you want to say?"));
				else
				{
					if(m_Buffer)
					{
						Payload.pop_front();
						QString msg = Payload.join(" ");

						m_Buffer->message(msg);
						showTextCurrentTab(tr("You whisper to your friends: %1").arg(msg));
					}
				}
			}
		}

		else
		{
			if(channel)
				channel->showText(tr("Invalid command."));
		}
	}

	else
	{
		if(channel)
			channel->message(Message);
	}
}

void IrcHandler::irc_buffer_added(Irc::Buffer *buffer)
{
	QString name = buffer->receiver();

	if(name == "irc.dotacash.com")
	{
		connect(buffer, SIGNAL(numericMessageReceived(const QString&, uint, const QStringList&)), this, SLOT(numericMessageReceived(const QString&, uint, const QStringList&)));
		emit showMessage(tr("Connected to %1").arg("irc.dotacash.com"), 3000);
	}

	else if(name.toLower() == m_Username.toLower())
	{
		connect(buffer, SIGNAL(messageReceived(const QString, const QString, Irc::Buffer::MessageFlags)), this, SLOT(messageReceived(const QString, const QString, Irc::Buffer::MessageFlags)));
	}

	else if(name.startsWith("##"))
	{
		if(name.toLower() != ("##" + m_Username.toLower()))
			m_FriendsMap[name.mid(2).toLower()] = new FriendsHandler(buffer, name.mid(2), this);
		else
			m_Buffer = buffer;
	}

	else
	{
		ChannelHandler* handler = new ChannelHandler(buffer, this);

		m_ChannelMap[name.toLower()] = handler;

		this->addTab(handler->GetTab(), name);

		int idx = this->count() - 1;

		if(name.toLower() == "#dotacash" || name.toLower() == "#dcchat")
			this->tabBar()->setTabButton(idx, QTabBar::RightSide, 0);

		this->setCurrentIndex(idx);
	}
}

void IrcHandler::irc_buffer_removed(Irc::Buffer *buffer)
{
	QString name = buffer->receiver();
	removeTabName(name);
}

void IrcHandler::messageReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags)
{
	ChannelHandler* handler = m_ChannelMap[origin.toLower()];
	
	if(!handler)
	{
		handler = new ChannelHandler(NULL, this);

		m_ChannelMap[origin.toLower()] = handler;

		this->addTab(handler->GetTab(), origin);
		this->setCurrentIndex(this->count() - 1);		
	}

	QString txt = QString("<%1> %2").arg(origin).arg(message);
	handler->showText(txt);
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

		chan->GetBuffer()->deleteLater();
		delete chan;
		m_ChannelMap[name.toLower()] = NULL;
	}
}

void IrcHandler::numericMessageReceived(const QString& origin, uint code, const QStringList& params)
{
	if(code == Irc::Rfc::ERR_NICKNAMEINUSE)
	{
		emit showMessage(tr("Nickname in use! Trying alternative nickname."));

		irc->setNick(QString("%1_").arg(irc->nick()));
		//irc->reconnectToServer();
	}

	else if(code == Irc::Rfc::RPL_NAMREPLY)
	{
		QString name = params.at(2).toLower();

		if(name.startsWith("##"))
		{
			if(name.mid(2) != m_Username.toLower())
			{
				FriendsHandler* target = m_FriendsMap[name.mid(2)];

				if(target)
				{
					if(target->GetBuffer()->names().contains(name.mid(2), Qt::CaseInsensitive))
						target->SetStatus(true);
				}
			}
		}
		else
		{
			ChannelHandler* target = m_ChannelMap[name];

			if(target)
				target->UpdateNames();
		}
	}
}

void IrcHandler::showTextCurrentTab(QString message)
{
	QString channel = this->tabText(this->currentIndex());
	ChannelHandler* handler = m_ChannelMap[channel.toLower()];

	if(handler)
		handler->showText(message);
}