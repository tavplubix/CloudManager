#pragma once
#include "CommonIncludes.h"
#include "FileClasses.h"
#include "ConfigFile.h"
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
	//QStringList localFiles, cloudFiles;			//FIXME вмето этой поебени использовать два конфиг-файла: локальный и удалённый; мержить их при синхронизации, заливать результат в облако
	//QStringList changedFiles, newLocalFiles, newCloudFiles;
	QAbstractManager::ActionWithChanged action;
	mutable QFile log;
	//===================== Private Methods ===========================
	//void compareFiles(); 
	const QNetworkReply* downloadFile(const ShortName& name, QIODevice* file) { QSharedPointer<QIODevice> tmp(file);  return downloadFile(name, tmp); }
	virtual void pushChangedFiles();
	virtual void pullChangedFiles();
	//void updateConfig();
protected:
	//===================== Protected Fields ==========================
	QAbstractManager::Status m_status;
	QSettings *settings;
	//===================== Protected Methods =========================
	void netLog(QNetworkReply *reply) const;
	void netLog(const QByteArray& type, const QNetworkRequest &request, const QByteArray &body) const;
	void waitFor(const QNetworkReply* reply) const;
	//============== Pure Virtual Prorected Methods ===================
	//public://dbg
	virtual bool authorized() const = 0;
	virtual const QNetworkReply* authorize() = 0;
	virtual const QNetworkReply* downloadFile(const ShortName& name, QSharedPointer<QIODevice> file) = 0;
	virtual const QNetworkReply* uploadFile(const ShortName& name, QIODevice* file) = 0;		//становится влдельцем file
	virtual const QNetworkReply* remove(const ShortName& name) = 0;
	//блокировать один файл или весть сервис целиком? (блокировка файла конфигурации == блокировка сервиса)
	virtual const QNetworkReply* lockFile(const ShortName& name /*= configFileName*/) { qDebug() << "WARNING: lockFile() is not implemented\n"; }
	virtual const QNetworkReply* unlockFile(const ShortName& name /*= configFileName*/) { qDebug() << "WARNING: unlockFile() is not implemented\n"; }
	virtual bool fileLocked(const ShortName& name /*= configFileName*/) const { qDebug() << "WARNING: fileLocked() is not implemented\n"; }
	virtual QString managerID() const = 0;		//костыль 
public:
	//====================== Public Methods ===========================
	QAbstractManager();
	virtual void init();	//нужна чтобы не ебаться с виртуальным конструктором
	QAbstractManager::Status status() const;
	void setActionWithChanged(QAbstractManager::ActionWithChanged action);
	QStringList managedFiles() const;
	virtual ~QAbstractManager();
	// ===================== Virtual Public Methods ===================
	virtual void syncAll();
	virtual void downloadAllNew();
	virtual void uploadAllNew();
	virtual void addFile(QFileInfo);
	virtual void removeFile(QFileInfo);		//removes file from this cloud only
	virtual void removeFileData(QFileInfo);		//removes file from all devices
	//============ Pure Virtual Public Methods ========================
	virtual qint64 spaceAvailable() const = 0;		//in bites
	virtual QDateTime lastModified(const ShortName& name) const = 0;
	


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













