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

#include "downloaderform.h"

#include <QMessageBox>
#include <QFileInfo>

DownloaderForm::DownloaderForm(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags)
{
	ui.setupUi(this);

	ui.progressBar->setValue(0);

	connect(&manager, SIGNAL(finished(QNetworkReply*)),
		SLOT(downloadFinished(QNetworkReply*)));

	downloadedCount = 1;
	totalCount = 0;
}

void DownloaderForm::addFile(QString url)
{
	downloadQueue.enqueue(url);

	totalCount++;
}

void DownloaderForm::beginDownloads()
{
	this->show();

	QTimer::singleShot(0, this, SLOT(startNextDownload()));
}

void DownloaderForm::startNextDownload()
{
	if (downloadQueue.isEmpty()) {
		emit downloadsComplete();
		return;
	}

	QUrl url = downloadQueue.dequeue();

	QString filename = saveFileName(url);
	output.setFileName("updates/" + filename);

	if (!output.open(QIODevice::WriteOnly)) {
		QMessageBox::warning(this, tr("Update Error"), tr("Problem opening save file '%1' for download '%2': %3")
			.arg(qPrintable(filename))
			.arg(url.toEncoded().constData())
			.arg(qPrintable(output.errorString())));

		downloadedCount++;
		startNextDownload();
		return;                 // skip this download
	}

	QNetworkRequest request(url);
	currentDownload = manager.get(request);
	connect(currentDownload, SIGNAL(downloadProgress(qint64,qint64)),
		SLOT(downloadProgress(qint64,qint64)));
	connect(currentDownload, SIGNAL(finished()),
		SLOT(downloadFinished()));
	connect(currentDownload, SIGNAL(readyRead()),
		SLOT(downloadReadyRead()));

	downloadTime.start();
}

QString DownloaderForm::saveFileName(const QUrl &url)
{
	QString path = url.path();
	QString basename = QFileInfo(path).fileName();

	if (basename.isEmpty())
		basename = "download";

	if (QFile::exists(basename)) {
		// already exists, don't overwrite
		int i = 0;
		basename += '.';
		while (QFile::exists(basename + QString::number(i)))
			++i;

		basename += QString::number(i);
	}

	return basename;
}

void DownloaderForm::downloadFinished()
{
	downloadedCount++;
	output.close();

	if (currentDownload->error())
	{
		QFile::remove(output.fileName());
		QMessageBox::warning(this, tr("Update Error"), tr("Failed: %1").arg(qPrintable(currentDownload->errorString())));
	}

	currentDownload->deleteLater();
	startNextDownload();
}

void DownloaderForm::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	ui.progressBar->setMaximum(bytesTotal);
	ui.progressBar->setValue(bytesReceived);

	// calculate the download speed
	double speed = bytesReceived * 1000.0 / downloadTime.elapsed();
	QString unit;
	if (speed < 1024) {
		unit = "bytes/sec";
	} else if (speed < 1024*1024) {
		speed /= 1024;
		unit = "kB/s";
	} else {
		speed /= 1024*1024;
		unit = "MB/s";
	}

	ui.lblStatus->setText(tr("Downloading file %1 at %2 %3").arg(currentDownload->url().toString()).arg(speed, 3, 'f', 1).arg(unit));
	ui.progressBar->update();

	ui.label->setText(tr("Downloading Updates (%1 of %2)").arg(downloadedCount).arg(totalCount));
}

void DownloaderForm::downloadReadyRead()
{
	output.write(currentDownload->readAll());
}
