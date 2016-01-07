#include "OAuth.h"

Token::Token()
{
	valid = false;
}

OAuthClientInfo::OAuthClientInfo(const QString& clientID, const QString& clientSecret, const QString& deviceID /*= QString()*/, const QString& deviceName /*= QString()*/)
	: id(clientID), secret(clientSecret), deviceID(deviceID), deviceName(deviceName)
{

}

OAuthServerInfo::OAuthServerInfo(const QUrl& codeRequestUrl, const QUrl& tokenRequestUrl, const QString& serviceName /*= QString()*/)
	: codeRequestUrl(codeRequestUrl), tokenRequestUrl(tokenRequestUrl), serviceName(serviceName)
{
	if (this->serviceName.isEmpty())
		this->serviceName = tokenRequestUrl.host();
}


OAuth2::OAuth2(const OAuthClientInfo& client, const OAuthServerInfo& server, const QString& userName /*= QString()*/)
	: client(client), server(server)
{
	m_token.userName = userName;
	m_token.serviceName = server.serviceName;
	oauthApiServer.connectToHostEncrypted(server.tokenRequestUrl.host());
	//tryReadTokenFromQSettings();
}

/*bool OAuth2::tryReadTokenFromQSettings()
{
	QSettings settings;
	settings.beginGroup("OAuth2");
	auto groups = settings.childGroups();
	if (groups.contains(m_token.serviceName) == true) {
		settings.beginGroup(m_token.serviceName);
		groups = settings.childGroups();
		if (groups.contains(m_token.userName) == true) {
			settings.beginGroup(m_token.userName);
			if (settings.childKeys().contains("token")) {
				m_token.value = settings.value("token").toByteArray();
				m_token.valid = true;
			}
			settings.endGroup();
		}
		settings.endGroup();
	}
	settings.endGroup();
	return m_token.valid;
}*/



void OAuth2::requestNewToken(FlowType /*= FlowType::AuthorizationCode*/)		//TODO do all this stuff in another thread
{
	//Get verification code
	QString label = QString::fromLocal8Bit("Введите код верификации. Его можно получить <a href=");
	label += "\"" + server.codeRequestUrl.toString() + "\"" + QString::fromLocal8Bit(">здесь</a>.\n");
	int verificationCode = QInputDialog::getInt(qApp->activeWindow(), QString::fromLocal8Bit("Авторизация"), label, -1, 0);
	if (verificationCode < 0)
		throw new AccessRejectedByUserException;

	//Get token by code
	QByteArray requestBody;
	requestBody += "grant_type=authorization_code";
	requestBody += "&code=" + QString::number(verificationCode);
	requestBody += "&client_id=" + client.id;
	requestBody += "&client_secret=" + client.secret;
	requestBody += "&device_id=" + client.deviceID;
	requestBody += "&device_name=" + client.deviceName;

	QNetworkRequest request(server.tokenRequestUrl);
	QNetworkReply* reply = oauthApiServer.post(request, requestBody);
	connect(&oauthApiServer, &QNetworkAccessManager::finished, this, &OAuth2::processResponse);
}

QByteArray OAuth2::authtorizationHeader()
{
	return "OAuth " + m_token.value;
}

Token OAuth2::token()
{
	return m_token;
}

void OAuth2::processResponse(QNetworkReply* reply)
{
	try {
		checkForHTTPErrors_maythrow(reply);
	}
	catch (UnexpectedHttpStatusCode* e) {
		qDebug() << "in OOAuth2::processResponse(): unexpected HTTP status code " << e->statusCode();
		emit error();
		return;
	}
	QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
	reply->deleteLater();
	if (response.contains("access_token")) {
		m_token.value = response["access_token"].toString().toLocal8Bit();
		m_token.valid = true;
		m_token.validUntil = time(nullptr) + response["expires_in"].toInt();
		emit tokenReady(m_token);
	}
	else {
		qDebug() << "Token request: Error: " << response["error_description"].toString() << " Code: " << response["error"].toString();
		emit error();
	}
}




