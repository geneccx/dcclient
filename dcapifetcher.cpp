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

#include "dcapifetcher.h"

ApiFetcher::ApiFetcher(QWidget* parent) : QObject(parent), currentReply(0)
{
	connect(&manager, SIGNAL(finished(QNetworkReply*)),
			this, SLOT(finished(QNetworkReply*)));

	connect(&manager, SIGNAL(finished(QNetworkReply*)),
			this, SLOT(deleteLater()));
}

ApiFetcher::~ApiFetcher()
{

}

void ApiFetcher::fetch(QString url)
{
	QUrl nUrl(url);
	get(nUrl);
}

void ApiFetcher::get(const QUrl &url)
{
	QNetworkRequest request(url);
	if (currentReply)
	{
		currentReply->disconnect(this);
		currentReply->deleteLater();
	}

	currentReply = manager.get(request);
	connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(currentReply, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
	connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}

void ApiFetcher::metaDataChanged()
{
	QUrl redirectionTarget = currentReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
	if (redirectionTarget.isValid()) {
		get(redirectionTarget);
	}
}

void ApiFetcher::readyRead()
{
	int statusCode = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (statusCode >= 200 && statusCode < 300)
	{
		QByteArray data = currentReply->readAll();
		QString str(data);

		result += str;
	}
}

void ApiFetcher::error(QNetworkReply::NetworkError)
{
	qWarning("apifetcher network error");
	currentReply->disconnect(this);
	currentReply->deleteLater();
	currentReply = 0;
}

void ApiFetcher::finished(QNetworkReply *reply)
{
	Q_UNUSED(reply);

	emit fetchComplete(result);
}
