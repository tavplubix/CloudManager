#pragma once
#include "CommonIncludes.h"
#include <QObject>
//================ Network Includes ======================
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>


#define WAIT_FOR(action) action;

class QAbstractManager 
	: public QObject
{
	Q_OBJECT
public:
	enum class Status { Init, WaitingForSettings, Authorization, Syncronizing, Ready, Failed };
	enum class ActionWithChanged { Push, Pull, SaveNewest, Nothing, Other };
private:
	//===================== Private Fields ============================
	QStringList localFiles, cloudFiles;			//FIXME вмето этой поебени использовать два конфиг-файла: локальный и удалённый; мержить их при синхронизации, заливать результат в облако
	QStringList changedFiles, newLocalFiles, newCloudFiles;
	QAbstractManager::ActionWithChanged action;
	mutable QFile log;
	//===================== Private Methods ===========================
	void compareFiles(); 
	virtual void pushChangedFiles();
	virtual void pullChangedFiles();
protected:
	//===================== Protected Fields ==========================
	const QDir rootDir = "F:/Backups/CloudManager/";	//Hardcoded temporary
	QAbstractManager::Status m_status;
	QSettings *settings;
	//===================== Protected Methods =========================
	void netLog(QNetworkReply *reply) const;
	void netLog(const QByteArray& type, const QNetworkRequest &request, const QByteArray &body) const;
	//============== Pure Virtual Prorected Methods ===================
	//public://dbg
	virtual bool authorized() const = 0;
	virtual QNetworkReply* authorize() = 0;
	virtual QNetworkReply* downloadFile(const QString& name, QIODevice* file) = 0;
	virtual QNetworkReply* uploadFile(const QString& name, QIODevice* file) = 0;
	virtual QNetworkReply* remove(const QString& name) = 0;
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
	virtual void removeFile(QFileInfo);
	//============ Pure Virtual Public Methods ========================
	virtual qint64 spaceAvailable() const = 0;		//in bites
	virtual QDateTime lastModified(QFileInfo) const = 0;
	


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













