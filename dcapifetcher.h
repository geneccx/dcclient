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
