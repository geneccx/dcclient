#include "xdcc.h"
#include "irchandler.h"
#include "channelhandler.h"

IrcHandler::IrcHandler(QString username, QTabWidget* parent) : QObject(parent),
	m_Username(username)
{
	irc = new Irc::Session(this);

	connect(parent, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
	connect(irc, SIGNAL(connected()), this, SLOT(irc_connected()));
	connect(irc, SIGNAL(disconnected()), this, SLOT(irc_disconnected()));
	connect(irc, SIGNAL(bufferAdded (Irc::Buffer*)), this, SLOT(irc_buffer_added(Irc::Buffer*)));
	connect(irc, SIGNAL(bufferRemoved (Irc::Buffer*)), this, SLOT(irc_buffer_removed(Irc::Buffer*)));

	irc->setNick(m_Username);
	irc->setIdent("dcclient");
	irc->setRealName(m_Username);
	irc->addAutoJoinChannel("#dcchat");
	irc->addAutoJoinChannel("#dotacash");
	irc->setAutoReconnectDelay(60);
	irc->connectToServer("irc.dotacash.com", 6667);
}

void IrcHandler::closeTab(int idx)
{
	QTabWidget* tabChannels = (QTabWidget*)parent();

	QString channel = tabChannels->tabText(idx);
	if(channel == "#dcchat" || channel == "#dotacash")
		return;

	else
	{
		if(channel.at(0) == '#')
			irc->part(channel);
		else
			removeTab(channel);
	}
}

void IrcHandler::irc_connected()
{
	emit showMessage(tr("Connecting to %1").arg("irc.dotacash.com"), 3000);
}

void IrcHandler::irc_disconnected()
{
	for(int i = 0; i < irc->buffers().size(); ++i)
	{
		irc_buffer_removed(irc->buffers().at(i));
		irc->buffers().at(i);
	}	

	for(int i = 0; i < channelMap.size(); ++i)
	{
		delete channelMap.values().at(i);
	}

	channelMap.clear();

	emit showMessage(tr("Disconnected from %1, reconnecting...").arg("irc.dotacash.com"), 3000);
}

void IrcHandler::handleChat(QString& origin, QString& Message)
{
	if(!irc)
		return;

	if(Message.isEmpty())
		return;

	ChannelHandler* channel = channelMap[origin.toLower()];

	if(Message.startsWith('/'))
	{
		QStringList Payload = Message.split(' ');
		QString Command;

		Command = Payload.at(0);
		Payload.pop_front();

		if((Command == "/join" || Command == "/j") && !Payload.isEmpty())
		{
			QString JoinChan = Payload.at(0);
			irc->join(JoinChan);
		}

		else if(Command == "/part" || Command == "/p")
		{
			if(Payload.isEmpty())
			{
				if(origin == "#dcchat" || origin == "#dotacash")
					return;

				irc->part(origin);
			}
			else
			{
				if(Payload.at(0) == "#dcchat" || Payload.at(0) == "#dotacash")
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

	else
	{
		ChannelHandler* handler = new ChannelHandler(buffer, (QWidget*)parent());

		channelMap[name.toLower()] = handler;
		QTabWidget* tabChannels = (QTabWidget*)parent();
		
		tabChannels->addTab(handler->GetTab(), name);
		tabChannels->setCurrentIndex(tabChannels->count() - 1);
	}
}

void IrcHandler::irc_buffer_removed(Irc::Buffer *buffer)
{
	QString name = buffer->receiver();
	removeTab(name);
}

void IrcHandler::messageReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags)
{
	ChannelHandler* handler = channelMap[origin.toLower()];
	
	if(!handler)
	{
		handler = new ChannelHandler(NULL, (QWidget*)parent());

		channelMap[origin.toLower()] = handler;
		QTabWidget* tabChannels = (QTabWidget*)parent();

		tabChannels->addTab(handler->GetTab(), origin);
		tabChannels->setCurrentIndex(tabChannels->count() - 1);		
	}

	QString txt = QString("<%1> %2").arg(origin).arg(message);
	handler->showText(txt);
}

void IrcHandler::removeTab(QString name)
{
	ChannelHandler* chan = channelMap[name.toLower()];
	QTabWidget* tabChannels = (QTabWidget*)parent();

	if(chan)
	{
		for(int i = 0; i < tabChannels->count(); i++)
		{
			if(tabChannels->tabText(i).toLower() == name.toLower())
			{
				tabChannels->removeTab(i);
				break;
			}
		}

		chan->GetBuffer()->deleteLater();
		delete chan;
		channelMap[name.toLower()] = NULL;
	}
}

void IrcHandler::numericMessageReceived(const QString& origin, uint code, const QStringList& params)
{
	if(code == Irc::Rfc::ERR_NICKNAMEINUSE)
	{
		emit showMessage(tr("Nickname in use! Trying alternative nickname."));

		irc->setNick(QString("%1_").arg(irc->nick()));
		irc->reconnectToServer();
	}

	else if(code == Irc::Rfc::RPL_NAMREPLY)
	{
		ChannelHandler* target = channelMap[params.at(2)];

		if(target)
			target->UpdateNames();
	}
}