#pragma once
#include "CommonIncludes.h"
#include <QObject>
//================ Network Includes ======================
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <memory>

#define DEBUG 0
#define NETLOG 0

#if DEBUG
#define private public
#define protected public
#endif

//#define NEW_SHARED(t, v) [&]()->QSharedPointer<t>{return QSharedPointer<t>(new t(v));}()

//EXCEPTIONS
class FatalError : QException {};
class NotAuthorized : QException {};	//TODO throw this exception if HTTP status code is 401 or 403 
class ShortName;
class LongName;
class ConfigFile;

typedef const QNetworkReply* ReplyID;


class QAbstractManager 
	: public QObject
{
	Q_OBJECT
public:
	enum class Status { Init, WaitingForSettings, Authorization, Syncronizing, Ready, Failed };
	enum class ActionWithChanged { Push, Pull, SaveNewest, Nothing, Other };
	friend class ConfigFile;
private:
	//===================== Private Fields ============================
	QSet<ShortName> local, remote;
	mutable QSet<ReplyID> replies;
	//QStringList localFiles, cloudFiles;			//FIXME вмето этой поебени использовать два конфиг-файла: локальный и удалённый; мержить их при синхронизации, заливать результат в облако
	//QStringList changedFiles, newLocalFiles, newCloudFiles;
	QAbstractManager::ActionWithChanged action;
	mutable QFile log;
	ConfigFile *config;
	//===================== Private Methods ===========================
	//void compareFiles(); 
	ReplyID downloadFile(const ShortName& name, QIODevice* file) { QSharedPointer<QIODevice> tmp(file);  return downloadFile(name, tmp); }
	//virtual void pushChangedFiles();
	//virtual void pullChangedFiles();
	//void updateConfig();
protected:
	//===================== Protected Fields ==========================
	QAbstractManager::Status m_status;
	QSettings *settings;
	//===================== Protected Methods =========================
	void netLog(QNetworkReply *reply) const;
	void netLog(const QByteArray& type, const QNetworkRequest &request, const QByteArray &body) const;
	void waitFor(ReplyID reply) const;
	void registerReply(ReplyID reply) const;
	void setReplyFinished(ReplyID reply) const;
	bool replyFinished(ReplyID reply) const;
	//============== Pure Virtual Prorected Methods ===================
	//public://dbg
	virtual bool authorized() const = 0;
	virtual ReplyID authorize() = 0;
	virtual ReplyID downloadFile(const ShortName& name, QSharedPointer<QIODevice> file) = 0;
	virtual ReplyID uploadFile(const ShortName& name, QIODevice* file) = 0;		//становится влдельцем file
	virtual ReplyID remove(const ShortName& name) = 0;
	//блокировать один файл или весть сервис целиком? (блокировка файла конфигурации == блокировка сервиса)
	virtual ReplyID lockFile(const ShortName& name /*= configFileName*/) { qDebug() << "WARNING: lockFile() is not implemented\n"; return nullptr; }
	virtual ReplyID unlockFile(const ShortName& name /*= configFileName*/) { qDebug() << "WARNING: unlockFile() is not implemented\n"; return nullptr; }
	virtual bool fileLocked(const ShortName& name /*= configFileName*/) const { qDebug() << "WARNING: fileLocked() is not implemented\n"; return false; }
	virtual QString managerID() const = 0;		//костыль 
public:
	//====================== Public Methods ===========================
	QAbstractManager();
	virtual void init();	//нужна чтобы не ебаться с виртуальным конструктором
	QAbstractManager::Status status() const;
	void setActionWithChanged(QAbstractManager::ActionWithChanged action);
	QList<LongName> managedFiles() const;
	QByteArray localMD5FileHash(const LongName& filename);
	virtual ~QAbstractManager();
	// ===================== Virtual Public Methods ===================
	virtual void syncAll();
	//virtual void downloadAllNew();
	//virtual void uploadAllNew();
	virtual void addFile(QFileInfo);
	virtual void removeFile(QFileInfo);		//removes file from this cloud only
	virtual void removeFileData(QFileInfo);		//removes file from all devices
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
	void done();		//костыль
};

//Yandex.Disk			WebDAV		REST
//Microsoft OneDrive	WebDAV?		REST
//Google Drive						REST
//Dropbox							REST













