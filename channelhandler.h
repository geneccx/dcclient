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

#ifndef CHANNELHANDLER_H
#define CHANNELHANDLER_H

#include "xdcc.h"
#include <QObject>
#include <QTextBrowser>
#include <QListWidget>

#include <Irc>
#include <IrcMessage>
#include <ircutil.h>

class ChannelHandler : public QObject
{
	Q_OBJECT

public:
	ChannelHandler(bool showUserlist=true, QWidget *parent=0);

	QWidget* GetTab() { return m_Tab; }

	void showText(QString msg) { txtChat->append(msg); }
	void UpdateNames(QStringList& names);
	void reloadSkin();

	void joined(const QString);
	void parted(const QString, const QString);

	void nickChanged(const QString, const QString);
	void messageReceived(const QString &origin, const QString &message);
	void noticeReceived(const QString &origin, const QString &message);

private:
	QWidget* m_Tab;

	QTextBrowser* txtChat;
	QListWidget* lstUsers;
};

#endif
