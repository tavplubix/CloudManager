#pragma once
#include "CommonIncludes.h"
#include <QObject>
//================ Network Includes ======================
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <memory>

#define DEBUG 1
#define NETLOG 0

#if DEBUG
#define private public
#define protected public
#endif

//#define NEW_SHARED(t, v) [&]()->QSharedPointer<t>{return QSharedPointer<t>(new t(v));}()

//EXCEPTIONS
class FatalError : QException {};
class NotAuthorized : QException {};	//TODO throw this exception if HTTP status code is 401 or 403 
class Error : QException {};
class InvalidPath : QException {};
class ShortName;
class LongName;
class ConfigFile;

typedef const QNetworkReply* ReplyID;


class AbstractCloud 
	: public QObject
{
	Q_OBJECT
public:
	enum class Status { Init, WaitingForSettings, Authorization, Syncronizing, Ready, Failed };
	enum class ActionWithChanged { Push, Pull, SaveNewest, Nothing, Other };
	friend class ConfigFile;
	friend class CustomRequestDialog;	//for testing
private:
	//===================== Private Fields ============================
//	mutable QSet<ReplyID> replies;
	AbstractCloud::ActionWithChanged action;
	mutable QFile log;
	ConfigFile *config;
	//===================== Private Methods ===========================
	ReplyID downloadFile(const ShortName& name, QIODevice* file) { QSharedPointer<QIODevice> tmp(file);  return downloadFile(name, tmp); }
protected:
	//===================== Protected Fields ==========================
	AbstractCloud::Status m_status;
	//QSettings *settings;
	const QString qsettingsGroup;
	//===================== Protected Methods =========================
	void netLog(QNetworkReply *reply, QIODevice* logDevice = nullptr) const;
	void netLog(const QByteArray& type, const QNetworkRequest &request, const QByteArray &body, QIODevice* logDevice = nullptr) const;
//	void registerReply(ReplyID reply) const;
	void waitFor(ReplyID reply) const;		//waits for the QNetworkReply::destroyed signal (it means that processing of a reply is complete)
	void waitForFinishedSignal(QNetworkReply* reply) const;
//protected slots:
//	void setReplyFinished(ReplyID reply) const;
//protected:
//	bool replyFinished(ReplyID reply) const;
//	QSet<ReplyID> allReplies() const { return replies; }
	//============== Pure Virtual Prorected Methods ===================
	virtual bool authorized() const = 0;
	virtual ReplyID authorize() = 0;
	virtual ReplyID downloadFile(const ShortName& name, QSharedPointer<QIODevice> file) = 0;
	virtual ReplyID uploadFile(const ShortName& name, QIODevice* file) = 0;		//становится влдельцем QIODevice* file
	virtual ReplyID remove(const ShortName& name) = 0;
	//блокировать один файл или весть сервис целиком? (блокировка файла конфигурации == блокировка сервиса)
	virtual ReplyID lockFile(const ShortName& name /*= configFileName*/) { qDebug() << "WARNING: lockFile() is not implemented\n"; return nullptr; }
	virtual ReplyID unlockFile(const ShortName& name /*= configFileName*/) { qDebug() << "WARNING: unlockFile() is not implemented\n"; return nullptr; }
	virtual bool fileLocked(const ShortName& name /*= configFileName*/) const { qDebug() << "WARNING: fileLocked() is not implemented\n"; return false; }
	//virtual QString managerID() const = 0;		//костыль 
public:
	//====================== Public Methods ===========================
	AbstractCloud(QString qsettingsGroup = QString());
	virtual void init();	//нужна чтобы не ебаться с виртуальным конструктором
	AbstractCloud::Status status() const;
	void setActionWithChanged(AbstractCloud::ActionWithChanged action);
	QList<LongName> managedFiles() const;
	QByteArray localMD5FileHash(const LongName& filename);
	void removeFile(QFileInfo);		//removes file from this cloud only
	void removeFileData(QFileInfo);		//removes file from all devices
	void addFile(QFileInfo);
	void syncAll();
	// ===================== Virtual Public Methods ===================
	virtual ~AbstractCloud();
	virtual QString serviceName() const { return "unknown"; };
	virtual QString userName() const { return "unknown"; };
#if DEBUG
	virtual QByteArray sendDebugRequest(QByteArray requestType, QString url, QByteArray body = QByteArray(),
		QList<QNetworkReply::RawHeaderPair> additionalHeaders = QList<QNetworkReply::RawHeaderPair>()) = 0;
#endif
	//============ Pure Virtual Public Methods ========================
	virtual qint64 spaceAvailable() const = 0;		//in bites
	virtual QDateTime lastModified(const ShortName& name) const = 0;
	virtual QByteArray remoteMD5FileHash(const ShortName& filename) const = 0;



	//=================== Prorected Qt Slots ==========================
protected slots:

	//======================== Qt Signals =============================
signals :
	void newFilesInTheCloud(QList<QFileInfo>);
	void statusChanged();
	//void done();		//CRUTCH
};

//Yandex.Disk			WebDAV		REST
//Microsoft OneDrive	WebDAV?		REST
//Google Drive						REST
//Dropbox							REST













