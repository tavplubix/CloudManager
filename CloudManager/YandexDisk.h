#pragma once

#include <QJsonDocument>
#include <QJsonObject>
//#include "WebDAVManager.h"
#include "AbstractCloud.h"
#include "OAuth.h"

#define checkForHTTPErrors(x) _checkForHTTPErrors(x, __FILE__, __LINE__, QT_MESSAGELOG_FUNC); 
#define HTTPstatus(reply) reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()

class __declspec(deprecated) YandexDiskWebDav :
	//public WebDAVManager
	public AbstractCloud
{
	Q_OBJECT
public:
	enum class AuthType : int { Basic, OAuth };
private:
	//==================== Hardcoded Const Private Fields ========================
	const QString appID = "40881b3716704a989c757c30b9922ff2";
	static const QString clientSecret;
	const QString callbackURL = "https://oauth.yandex.ru/verification_code";
	//====================== Private Fields ======================================
	mutable QNetworkAccessManager disk, *oauth;	
	AuthType authtype;
	QByteArray authorizationHeader;
	time_t validUntil;
	//====================== Private Methods =====================================
	bool _checkForHTTPErrors(QNetworkReply *reply, const char* file, const int line, const char* func) const;
	void createDirIfNecessary(ShortName dirname);
protected:
	//======================== Virtual Protected Methods =========================
	bool authorized() const override final;
	Request authorize() override final;
	//QString managerID() const override { return "42"; };
	qint64 m_spaceAvailable() const override final;
	Request downloadFile(const ShortName& name, QSharedPointer<QIODevice> file) override final;
	Request uploadFile(const ShortName& name, QIODevice* file) override final;
	Request remove(const ShortName& name) override final;
	virtual void mkdir(ShortName dir);
public:
	//=========================== Public Methods =================================
	YandexDiskWebDav(QString qsettingsGroup = QString());
	~YandexDiskWebDav();
	YandexDiskWebDav::AuthType authType() const { return authtype; }
	void setAuthType(YandexDiskWebDav::AuthType type);

	QDateTime lastModified(const ShortName& name) const override final;
	QByteArray remoteMD5FileHash(const ShortName& filename) const override final;
	QString serviceName() const override { return "YandexDisk"; };
	QString userName() const override;
	QByteArray sendDebugRequest(QByteArray requestType, QString url, QByteArray body = QByteArray(),
		QList<QNetworkReply::RawHeaderPair> additionalHeaders = QList<QNetworkReply::RawHeaderPair>()) override final;

	//=========================== Private Qt Slots ===============================
private slots:
	//void tokenGot();
	//void fileDownloaded();
	//=============================== Qt Signals =================================
signals :

};

