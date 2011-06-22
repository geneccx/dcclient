#ifndef IRCHANDLER_H
#define IRCHANDLER_H

#include <QTabWidget>
#include <QMap>
#include <QStringList>
#include <QUrl>

#define IRC_STATIC
#include <ircclient-qt/Irc>
#include <ircclient-qt/IrcBuffer>
#include <ircclient-qt/IrcSession>

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
	void part(QString channel) { if(irc) irc->part(channel); }

public slots:
	void irc_connected();
	void irc_disconnected();
	void irc_buffer_added(Irc::Buffer *buffer);
	void irc_buffer_removed(Irc::Buffer *buffer);

	void numericMessageReceived(const QString& origin, uint code, const QStringList& params);
	void messageReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags);
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
	Irc::Session *irc;
	Irc::Buffer *m_Buffer;

	QMap<QString, ChannelHandler*> m_ChannelMap;
	QMap<QString, FriendsHandler*> m_FriendsMap;

	QString m_Username;
	QStringList m_Friends;

	void removeTabName(QString name);
};

#endif