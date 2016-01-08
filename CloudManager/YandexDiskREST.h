#pragma once
#include "AbstractCloud.h"
#include "RequestManager.h"


#include <QJsonDocument>
#include <QJsonObject>

#include "OAuth.h"

#include "FileClasses.h"
 

class YandexDiskREST :
	public AbstractCloud
{
	Q_OBJECT
private:
	//==================== Hardcoded Const Private Fields ========================
	const QString appID = "40881b3716704a989c757c30b9922ff2";
	static const QString clientSecret;
	const QString callbackURL = "https://oauth.yandex.ru/verification_code";
	const QString apiURL = "https://cloud-api.yandex.net/v1/disk/";
	//====================== Private Fields ======================================
	mutable RequestManager api, disk;
	QByteArray authorizationHeader;
	//====================== Private Methods =====================================
	//void createDirIfNecessary(ShortName dirname);
protected:
	//======================== Virtual Protected Methods =========================
	bool authorized() const override final;
	ReplyID authorize() override final;
	qint64 m_spaceAvailable() const override final;
	ReplyID downloadFile(const ShortName& name, QSharedPointer<QIODevice> file) override final;
	ReplyID uploadFile(const ShortName& name, QIODevice* file) override final;
	ReplyID remove(const ShortName& name) override final;
	virtual void mkdir(const QString& dir);
public:
	//=========================== Public Methods =================================
	YandexDiskREST(QString qsettingsGroup = QString());
	~YandexDiskREST();

	QDateTime lastModified(const ShortName& name) const override final;
	QByteArray remoteMD5FileHash(const ShortName& filename) const override final;
	QString serviceName() const override { return "YandexDisk"; };
	QString userName() const override;
#if DEBUG
	QByteArray sendDebugRequest(QByteArray requestType, QString url, QByteArray body = QByteArray(),
		QList<QNetworkReply::RawHeaderPair> additionalHeaders = QList<QNetworkReply::RawHeaderPair>()) override final;
#endif

	//=========================== Private Qt Slots ===============================
	private slots:
	//void tokenGot();
	//void fileDownloaded();
	//=============================== Qt Signals =================================
	signals :

};








