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

#ifndef FRIENDSHANDLER_H
#define FRIENDSHANDLER_H

#include "xdcc.h"
#include <QObject>
#include <QTextBrowser>
#include <QListWidget>

#define COMMUNI_STATIC
#include <Irc>
#include <IrcMessage>

class FriendsHandler : public QObject
{
	Q_OBJECT

public:
	FriendsHandler(QString nFriendName, QWidget *parent=0);

	bool GetStatus() { return m_Status; }

	void SetStatus(bool nStatus) { m_Status = nStatus; }

public slots:
	void joined(const QString);
	void parted(const QString, const QString);

	void nickChanged(const QString, const QString);
	void messageReceived(const QString &origin, const QString &message);
	void noticeReceived(const QString &origin, const QString &message);

signals:
	void showMessage(QString, MessageType msgType = Normal);
	void requestGame(QString);

private:
	QString m_FriendName;

	bool m_Status;
};

#endif
