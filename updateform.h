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

#ifndef XDCC_UPDATE_H
#define XDCC_UPDATE_H

#include <QtGui/QMainWindow>
#include <QSettings>

#include "xdcc.h"
#include "ui_xdcc_update.h"

class DownloaderForm;

class UpdateForm : public QDialog
{
	Q_OBJECT

public:
	UpdateForm(QWidget *parent = 0, Qt::WFlags flags = 0);

	void checkForUpdates(QString appCastURL, bool alwaysShow);

public slots:
	void parseUpdateData(QString& data);
	void updateNow();
	void beginUpdate();

signals:
	void updateFromURL(QString&);

private:
	Ui_UpdateDialog ui;
	DownloaderForm* m_Downloader;

	QMap<QString,QString> m_Latest;
	ApiFetcher* m_Fetcher;
	QString m_Version;
	bool m_AlwaysShow;

    QMap<QString, QString> parseUpdateItem(QXmlStreamReader& xml);
	void addElementDataToMap(QXmlStreamReader& xml, QMap<QString, QString>& map);
	bool isUpdateRequired(QString& latestVer);	
};

#endif // XDCC_UPDATEFORM_H
