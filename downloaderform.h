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

#ifndef XDCC_DOWNLOADERFORM_H
#define XDCC_DOWNLOADERFORM_H

#include <QtGui/QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QQueue>
#include <QFile>
#include <QTime>

#include "xdcc.h"
#include "ui_xdcc_downloader.h"

class DownloaderForm : public QDialog
{
	Q_OBJECT

public:
	DownloaderForm(QWidget *parent = 0, Qt::WFlags flags = 0);

	void addFile(QString url);
	void beginDownloads();

public slots:
	void startNextDownload();
	void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void downloadFinished();
	void downloadReadyRead();

signals:
	void downloadsComplete();

private:
	Ui_DownloaderForm ui;

	QNetworkAccessManager manager;
	QQueue<QUrl> downloadQueue;
	QNetworkReply* currentDownload;
	QFile output;
	QTime downloadTime;
	int downloadedCount;
	int totalCount;

	QString DownloaderForm::saveFileName(const QUrl &url);
};

#endif // XDCC_DOWNLOADERFORM_H
