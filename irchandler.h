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

#ifndef IRCHANDLER_H
#define IRCHANDLER_H

#include <QTabWidget>
#include <QMap>
#include <QStringList>
#include <QUrl>

#define COMMUNI_STATIC
#include <Irc>
#include <IrcMessage>
#include <IrcSession>
#include <IrcCommand>
#include <ircprefix.h>

class ChannelHandler;
class FriendsHandler;

enum MessageType
{
	Normal,
	Sys,
	Err,
	Friends
};

class IrcHandler : public QTabWidget
{
	Q_OBJECT
public:
	IrcHandler(QWidget* parent);

	void connectToIrc(QString name);
	void part(QString channel) { if(irc) irc->sendCommand(IrcCommand::createPart(channel)); }

public slots:
	void connected();
	void disconnected();

	void messageReceived(IrcMessage *message);
	void handleChat(QString&, QString&);
	void myCloseTab(int);

	void showTextCurrentTab(QString, MessageType=Normal);
	void reloadSkin();

	void joinedGame(QString, QString);
	void handleUrl(QUrl);

signals:
	void showMessage(QString, int timeout=3000);
	void requestGame(QString);

private:
	IrcSession *irc;

	QMap<QString, ChannelHandler*> m_ChannelMap;
	QMap<QString, FriendsHandler*> m_FriendsMap;

	QString m_Username;
	QStringList m_Friends;

	void removeTabName(QString name);
	void joinedChannel(IrcPrefix origin, IrcJoinMessage* joinMsg);
	void partedChannel(IrcPrefix origin, IrcPartMessage* partMsg);
	void quitMessage(IrcPrefix origin, IrcQuitMessage* quitMsg);
	void privateMessage(IrcPrefix origin, IrcPrivateMessage* privMsg);
	void noticeMessage(IrcPrefix origin, IrcNoticeMessage* noticeMsg);
	void nickMessage(IrcPrefix origin, IrcNickMessage* nickMsg);
	void numericMessage(IrcNumericMessage* numMsg);
};

#endif
