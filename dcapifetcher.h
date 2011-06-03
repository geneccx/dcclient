#ifndef DCAPIFETCHER_H
#define DCAPIFETCHER_H

#include "xdcc.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ApiFetcher : public QObject
{
	Q_OBJECT

public:
	ApiFetcher(QWidget* parent=0);
	~ApiFetcher();

	void fetch(QString url);

signals:
	void fetchComplete(QString& data);

public slots:
	void finished(QNetworkReply *reply);
	void readyRead();
	void metaDataChanged();
	void error(QNetworkReply::NetworkError);

private:
	QNetworkAccessManager manager;
	QNetworkReply *currentReply;

	QString result;

	void get(const QUrl &url);
};

#endif