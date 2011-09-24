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
#include "dcapifetcher.h"
#include "xmlstructs.h"
#include "xdcc_version.h"

#include <QMessageBox>

UpdateForm::UpdateForm(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent, flags)
{
	ui.setupUi(this);

	connect(ui.okButton, SIGNAL(clicked()), this, SLOT(updateNow()));

	m_Version = QSettings("DotaCash", "DCClient X").value("version", XDCC_VERSION).toString();
}

void UpdateForm::updateNow()
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

	int i = 0;

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
				if(i < 2)
				{
					++i;
					continue;
				}
				m_Latest = parseUpdateItem(xml);
				break;
			}
		}
	}

	if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError)
	{
		qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
	}

	QStringList splitVersion = m_Version.split('.');
	if(!splitVersion.isEmpty())
	{
		int max = splitVersion.size();

		if(max > 3)
			max = 3;

		int ver[3] = {0};

		for(int i = 0; i < max; ++i)
			ver[i] = splitVersion[i].toInt();

		QStringList splitVersion2 = m_Latest["version"].split('.');
		
		int max2 = splitVersion2.size();

		if(max2 > 3)
			max2 = 3;

		int ver2[3] = {0};

		for(int i = 0; i < max2; ++i)
			ver2[i] = splitVersion2[i].toInt();

		bool isNewer = false;

		for(int i = 0; i < 3; ++i)
		{
			if(ver[i] > ver2[i])
				break;

			if(ver2[i] > ver[i])
			{
				isNewer = true;
				break;
			}
		}

		if(isNewer)
		{
			ui.label->setText("<span style=\" font-size:14pt; color:#17069c;\">" + m_Latest["title"] + "</span>");
			ui.textBrowser->setText(m_Latest["description"].replace("</br>", "<br>"));
			ui.textBrowser->resize(ui.textBrowser->sizeHint());
			ui.cancelButton->setText("Later");
			ui.okButton->show();
		}
		else
		{
			ui.label->setText("<span style=\" font-size:14pt; color:#17069c;\">" + tr("You're up to date!") + "</span>");
			ui.textBrowser->setText(tr("DCClient v%1 is the newest version currently available.").arg(m_Version));
			ui.cancelButton->setText("OK");
			ui.okButton->hide();
		}

		if(isNewer || m_AlwaysShow)
			this->show();
	}
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