#include <QObject>
#include "channelhandler.h"

ChannelHandler::ChannelHandler(Irc::Buffer* nBuffer, QWidget* parent) 
	: QObject(parent), m_Buffer(nBuffer)
{
	m_Tab = new QWidget(parent);

	QHBoxLayout* horizontalLayout = new QHBoxLayout(m_Tab);
	horizontalLayout->setSpacing(10);
	txtChat = new QTextBrowser(m_Tab);
	txtChat->setMinimumWidth(560);

	horizontalLayout->addWidget(txtChat);

	lstUsers = new QListWidget(m_Tab);
	lstUsers->setSortingEnabled(true);
	lstUsers->setMaximumWidth(180);
	lstUsers->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	lstUsers->setAlternatingRowColors(true);
	
	horizontalLayout->addWidget(lstUsers);

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

void ChannelHandler::UpdateNames()
{
	lstUsers->clear();
	lstUsers->addItems(m_Buffer->names());
}

void ChannelHandler::nickChanged(const QString user, const QString nick)
{
	QList<QListWidgetItem*> list = lstUsers->findItems(user, Qt::MatchFixedString);

	for(int i = 0; i < list.size(); i++)
	{
		list.at(i)->setText(nick);
	}

	txtChat->append(tr("%1 is now known as %2").arg(user).arg(nick));
}

void ChannelHandler::joined(const QString user)
{
	QList<QListWidgetItem*> list = lstUsers->findItems(user, Qt::MatchFixedString);
	if(list.isEmpty())
	{
		lstUsers->addItem(user);
	}	
}

void ChannelHandler::parted(const QString user, const QString reason)
{
	QList<QListWidgetItem*> list = lstUsers->findItems(user, Qt::MatchFixedString);

	for(int i = 0; i < list.size(); i++)
	{
		lstUsers->removeItemWidget(list.at(i));
		delete list.at(i);
	}
}

void ChannelHandler::messageReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags)
{
	QString txt = QString("<%1> %2").arg(origin).arg(message);
	txtChat->append(txt);
}

void ChannelHandler::noticeReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags)
{
	QString txt = QString("-%1- %2").arg(origin).arg(message);
	txtChat->append(txt);
}