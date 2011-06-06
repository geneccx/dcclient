#ifndef IRCHANDLER_H
#define IRCHANDLER_H

#include <QTabWidget>
#include <QMap>

#define IRC_STATIC
#include <ircclient-qt/Irc>
#include <ircclient-qt/IrcBuffer>
#include <ircclient-qt/IrcSession>

class ChannelHandler;

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

signals:
	void showMessage(QString, int timeout=3000);

private:
	Irc::Session *irc;

	QMap<QString, ChannelHandler*> channelMap;

	QString m_Username;

	void removeTabName(QString name);
};

#endif