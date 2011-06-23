#ifndef CHANNELHANDLER_H
#define CHANNELHANDLER_H

#include "xdcc.h"
#include <QObject>
#include <QTextBrowser>
#include <QListWidget>

#define IRC_STATIC
#include <ircclient-qt/Irc>
#include <ircclient-qt/IrcBuffer>

class ChannelHandler : public QObject
{
	Q_OBJECT

public:
	ChannelHandler(Irc::Buffer* nBuffer, QWidget *parent=0);

	QWidget* GetTab() { return m_Tab; }
	Irc::Buffer* GetBuffer() { return m_Buffer; }

	void showText(QString msg) { txtChat->append(msg); }
	void message(QString msg) { if(m_Buffer) m_Buffer->message(msg); }
	void UpdateNames();
	void reloadSkin();

public slots:
	void joined(const QString);
	void parted(const QString, const QString);

	void nickChanged(const QString, const QString);
	void messageReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags=Irc::Buffer::NoFlags);
	void noticeReceived(const QString &origin, const QString &message, Irc::Buffer::MessageFlags flags=Irc::Buffer::NoFlags);

private:
	Irc::Buffer* m_Buffer;
	QWidget* m_Tab;

	QTextBrowser* txtChat;
	QListWidget* lstUsers;
};

#endif
