#pragma once

#include <ctime>

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QSettings>
#include <QInputDialog>

#include <QJsonDocument>
#include <QJsonObject>

#include <QException>
#include <QApplication>


#include "NetworkExceptions.h"


struct Token {
	bool valid;
	time_t validUntil;
	QString userName, serviceName;
	QByteArray value;
	Token();
};

struct OAuthClientInfo {
	QString id;
	QString secret;
	QString deviceID;
	QString deviceName;
	OAuthClientInfo(const QString& clientID, const QString& clientSecret, const QString& deviceID = QString(), const QString& deviceName = QString());
};

struct OAuthServerInfo {
	QString serviceName;
	QUrl codeRequestUrl;
	QUrl tokenRequestUrl;
	OAuthServerInfo(const QUrl& codeRequestUrl, const QUrl& tokenRequestUrl, const QString& serviceName = QString());
};

class AccessRejectedByUserException : public QException {};

class OAuth2 : public QObject
{
	Q_OBJECT
	QNetworkAccessManager oauthApiServer;
	OAuthClientInfo client;
	OAuthServerInfo server;
	Token m_token;
	//bool tryReadTokenFromQSettings();
private slots:
	void processResponse(QNetworkReply* reply);
public:
	enum class FlowType {
		ImplicitGrant, AuthorizationCode
	};
	OAuth2(const OAuthClientInfo& client, const OAuthServerInfo& server, const QString& userName = QString());
	void requestNewToken(FlowType = FlowType::AuthorizationCode);
	QByteArray authtorizationHeader();
	Token token();

signals:
	void tokenReady(Token);
	void error();

};






