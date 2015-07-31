#pragma once
//#include "WebDAVManager.h"
#include "QAbstractManager.h"
#define checkForHTTPErrors(x) _checkForHTTPErrors(x, __FILE__, __LINE__, QT_MESSAGELOG_FUNC); 
#define HTTPstatus(reply) reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()

class YandexDiskManager :
	//public WebDAVManager
	public QAbstractManager
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
	QNetworkAccessManager disk, *oauth;	
	AuthType authtype;
	QByteArray authorizationHeader;
	time_t validUntil;
	//====================== Private Methods =====================================
	bool _checkForHTTPErrors(QNetworkReply *reply, const char* file, const int line, const char* func);
protected:
	//======================== Virtual Protected Methods =========================
	virtual bool authorized() override final;
	virtual void authorize() override final;
public:
	virtual qint64 spaceAvailable() override final;
public: //debug
	virtual void downloadFile(QFileInfo file) override final;
	virtual void uploadFile(QFileInfo file) override final;

	//============================ Protected Methods ==============================
	void mkdir(QDir dir);
	void remove(QFileInfo file);


public:
	//=========================== Public Methods =================================
	YandexDiskManager();
	~YandexDiskManager();
	YandexDiskManager::AuthType authType() { return authtype; }
	void setAuthType(YandexDiskManager::AuthType type);

	//=========================== Private Qt Slots ===============================
private slots:
	//void tokenGot();
	//void fileDownloaded();
	//=============================== Qt Signals =================================
signals :
	

};

