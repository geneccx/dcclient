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

#include "updateform.h"
#include "downloaderform.h"
#include "dcapifetcher.h"
#include "xdcc_version.h"

#include <QDir>

UpdateForm::UpdateForm(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags)
{
	ui.setupUi(this);

	connect(ui.okButton, SIGNAL(clicked()), this, SLOT(updateNow()));

	m_Version = QSettings("DotaCash", "DCClient X").value("version", XDCC_VERSION).toString();

	m_Downloader = new DownloaderForm(this);

	connect(m_Downloader, SIGNAL(downloadsComplete()), this, SLOT(beginUpdate()));
}

void UpdateForm::updateNow()
{
	QDir updateFolder("./updates/");

	QStringList list = updateFolder.entryList(QDir::Files);

	for (int i = 0; i < list.size(); ++i)
		updateFolder.remove(list.at(i));

	m_Downloader->addFile("http://www.dotacash.com/xdcc/file_list.xml");
	m_Downloader->addFile(m_Latest["url"]);
	m_Downloader->beginDownloads();
}

void UpdateForm::beginUpdate()
{
	emit updateFromURL(m_Latest["url"]);
}

void UpdateForm::checkForUpdates(QString appCastURL, bool alwaysShow)
{
	m_AlwaysShow = alwaysShow;

	ApiFetcher* updateFetcher = new ApiFetcher(this);
	connect(updateFetcher, SIGNAL(fetchComplete(QString&)), this, SLOT(parseUpdateData(QString&)));

	updateFetcher->fetch(appCastURL);
}

void UpdateForm::parseUpdateData(QString& data)
{
	QXmlStreamReader xml(data);

	while(!xml.atEnd() && !xml.hasError())
	{
		xml.readNext();

		if(xml.isStartDocument())
			continue;
		
		if(xml.isStartElement())
		{
			if(xml.name() == "channel")
				continue;

			if(xml.name() == "item")
			{
				m_Latest = parseUpdateItem(xml);
				break;
			}
		}
	}

	if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError)
	{
		qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
	}

	bool isNewer = isUpdateRequired(m_Latest["version"]);
	
	if(isNewer)
	{
		ui.label->setText("<span style=\" font-size:14pt; color:#17069c;\">" + m_Latest["title"] + "</span>");
		ui.lblChangelog->setText(m_Latest["description"].replace("</br>", "<br>"));

		ui.cancelButton->setText("Later");
		ui.okButton->show();
	}
	else
	{
		ui.label->setText("<span style=\" font-size:14pt; color:#17069c;\">" + tr("You're up to date!") + "</span>");
		ui.lblChangelog->setText(tr("DCClient v%1 is the newest version currently available.").arg(m_Version));
			
		ui.cancelButton->setText("OK");
		ui.okButton->hide();
	}

	if(isNewer || m_AlwaysShow)
		this->show();
}

bool UpdateForm::isUpdateRequired(QString& latestVer)
{
	QStringList vals1 = m_Version.split('.');
	QStringList vals2 = latestVer.split('.');

	int i=0;
	while(i < vals1.length() && i < vals2.length() && vals1[i] == vals2[i]) {
		i++;
	}

	if (i < vals1.length() && i < vals2.length())
	{
		if(vals1[i].toInt() < vals2[i].toInt())
			return true;
	}

	return false;
}

QMap<QString, QString> UpdateForm::parseUpdateItem(QXmlStreamReader& xml)
{
	QMap<QString, QString> mapData;

	if(!xml.isStartElement() || xml.name() != "item")
		return mapData;

	xml.readNext();

	while(!(xml.isEndElement() && xml.name() == "item"))
	{
		if(xml.isStartElement())
		{
			if(xml.name() == "channel")
				continue;

			if(xml.name() == "title")
				addElementDataToMap(xml, mapData);
			else if(xml.name() == "description")
				addElementDataToMap(xml, mapData);
			else if(xml.name() == "enclosure")
			{
				QXmlStreamAttributes attrs = xml.attributes();
				
				mapData["url"]		= attrs.value("url").toString();
				mapData["version"]	= attrs.value("sparkle:version").toString();
				mapData["os"]		= attrs.value("sparkle:os").toString();
				mapData["type"]		= attrs.value("type").toString();
				mapData["size"]		= attrs.value("size").toString();
			}
		}

		xml.readNext();
	}

	return mapData;
}

void UpdateForm::addElementDataToMap(QXmlStreamReader& xml, QMap<QString, QString>& map)
{
	if(!xml.isStartElement())
		return;
	
	QString elementName = xml.name().toString();

	xml.readNext();
		
	if(!xml.isCharacters() || xml.isWhitespace())
		return;
	
	map.insert(elementName, xml.text().toString());
}