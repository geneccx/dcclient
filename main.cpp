#include "xdcc.h"
#include "loginform.h"
#include <QtGui/QMainWindow>
#include <QSharedMemory>
#include <QTranslator>
#include <QLibraryInfo>
#include <QFile>

#include <qlocalserver.h>
#include <qlocalsocket.h>

#include <winsparkle.h>

QSharedMemory* _singular;

QtMsgHandler oldHandler = NULL;

void msgHandler(QtMsgType type, const char *msg)
{
	oldHandler(type, msg);

	QFile logfile("dcclog.txt");
	if(logfile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
	{
		QTextStream out(&logfile);
		out << msg << "\n";
		logfile.close();
	}
}

int main(int argc, char *argv[])
{
	//oldHandler = qInstallMsgHandler(msgHandler);

	QApplication a(argc, argv);

	QTranslator qtTranslator;
	qtTranslator.load(":/lang/qt_" + QLocale::system().name(),
		QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	a.installTranslator(&qtTranslator);

	QTranslator myappTranslator;
	myappTranslator.load(":/lang/xdcc_" + QLocale::system().name());
	a.installTranslator(&myappTranslator);

	_singular = new QSharedMemory("DCClient");

	if(_singular->attach(QSharedMemory::ReadWrite))
	{
		_singular->detach();

		if(argc >= 2)
		{
			QLocalSocket socket;
			socket.connectToServer("DCClientIPC");

			if(socket.waitForConnected())
			{
				QDataStream out(&socket);

				out << QString(argv[1]);

				socket.waitForBytesWritten();
			}
		}

		return 0;
	}

	_singular->create(1);

	xDCC xdcc;
	return a.exec();
}
