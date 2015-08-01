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
	QStringList localFiles, cloudFiles;
	QStringList changedFiles, newLocalFiles, newCloudFiles;
	QAbstractManager::ActionWithChanged action;
	mutable QFile log;
	//===================== Private Methods ===========================
	void compareFiles();
protected:
	const QDir rootDir = "F:/Backups/CloudManager/";	//Hardcoded temporary
	QAbstractManager::Status m_status;
	QSettings *settings;
	//===================== Protected Methods =========================
	void netLog(QNetworkReply *reply) const;
	void netLog(const QNetworkRequest &request, const QByteArray &body) const;
	//================= Virtual Prorected Methods =====================
	virtual void pushChangedFiles();
	virtual void pullChangedFiles();
	//============== Pure Virtual Prorected Methods ===================
	virtual void downloadFile(QFileInfo) = 0;
	virtual void uploadFile(QFileInfo) = 0;
	virtual bool authorized() const = 0;
	virtual void authorize() = 0;
	virtual void remove(QFileInfo) = 0;
	virtual QString managerID() const = 0;		//костыль

	//QSslConfiguration sslconf_DEBUG_ONLY();
	
public:
	//====================== Public Methods ===========================
	QAbstractManager();
	virtual void init();	//нужна чтобы не ебаться с виртуальным конструктором
	QAbstractManager::Status status() const;
	void setActionWithChanged(QAbstractManager::ActionWithChanged action);
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













