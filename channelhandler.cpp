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

#include <QObject>
#include <QFile>

#define COMMUNI_STATIC
#include <ircutil.h>

#include "channelhandler.h"


ChannelHandler::ChannelHandler(bool showUserlist, QWidget* parent) 
	: QObject(parent), lstUsers(0)
{
	m_Tab = new QWidget(parent);

	QHBoxLayout* horizontalLayout = new QHBoxLayout(m_Tab);
	horizontalLayout->setSpacing(10);
	txtChat = new QTextBrowser(m_Tab);
	txtChat->setMinimumWidth(560);
	txtChat->setOpenLinks(false);

	horizontalLayout->addWidget(txtChat);

	if(showUserlist)
	{
		lstUsers = new QListWidget(m_Tab);
		lstUsers->setSortingEnabled(true);
		lstUsers->setMaximumWidth(180);
		lstUsers->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		lstUsers->setAlternatingRowColors(true);

		horizontalLayout->addWidget(lstUsers);
	}

	connect(txtChat, SIGNAL(anchorClicked(QUrl)), parent, SLOT(handleUrl(QUrl)));

	reloadSkin();
}

void ChannelHandler::reloadSkin()
{
	QString skin = QSettings("DotaCash", "DCClient X", this).value("Skin", "default").toString();

	QFile styleSheet(QString("./skins/%1/style.css").arg(skin));
	QString style;

	if(styleSheet.open(QFile::ReadOnly))
	{
		QTextStream styleIn(&styleSheet);
		style = styleIn.readAll();
		styleSheet.close();

		txtChat->document()->setDefaultStyleSheet(style);
	}
}

void ChannelHandler::UpdateNames(QStringList& names)
{
	if(!lstUsers)
		return;

	lstUsers->addItems(names);
}

void ChannelHandler::nickChanged(const QString user, const QString nick)
{
	if(!lstUsers)
		return;

	QList<QListWidgetItem*> list = lstUsers->findItems(user, Qt::MatchFixedString);

	for(int i = 0; i < list.size(); i++)
	{
		list.at(i)->setText(nick);
	}

	txtChat->append(tr("<span class = \"sysmsg\">%1 is now known as %2</span>").arg(user).arg(nick));
}

void ChannelHandler::joined(const QString user)
{
	if(!lstUsers)
		return;

	QList<QListWidgetItem*> list = lstUsers->findItems(user, Qt::MatchFixedString);
	if(list.isEmpty())
	{
		lstUsers->addItem(user);
	}
}

void ChannelHandler::parted(const QString user, const QString reason)
{
	if(!lstUsers)
		return;

	QList<QListWidgetItem*> list = lstUsers->findItems(user, Qt::MatchFixedString);

	for(int i = 0; i < list.size(); i++)
	{
		lstUsers->removeItemWidget(list.at(i));
		delete list.at(i);
	}
}

void ChannelHandler::messageReceived(const QString &origin, const QString &message)
{
	QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
	QString txt = QString("<span class = \"msg\">[%3]&lt;%1&gt; %2</span>").arg(origin).arg(IrcUtil::messageToHtml(message)).arg(timestamp);
	txtChat->append(txt);
}

void ChannelHandler::noticeReceived(const QString &origin, const QString &message)
{
	QString timestamp = QDateTime::currentDateTime().toString("hh:mm");
	QString txt = QString("<span class = \"notice\">[%3] -%1- %2</span>").arg(origin).arg(IrcUtil::messageToHtml(message)).arg(timestamp);
	txtChat->append(txt);
}
