#include "xdcc.h"

#include "loginform.h"
#include <QtGui/QMainWindow>
#include <QSharedMemory>
#include <QTranslator>
#include <QLibraryInfo>
#include <QFile>

#include <qlocalserver.h>
#include <qlocalsocket.h>

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

#define _CRTDBG_MAP_ALLOC

int main(int argc, char *argv[])
{
	//_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF  | _CRTDBG_LEAK_CHECK_DF );


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

	XDCC xdcc;
	int ret = a.exec();

	delete _singular;

	return ret;
}
