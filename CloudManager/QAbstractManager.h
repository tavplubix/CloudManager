#pragma once
#include "CommonIncludes.h"
#include <QObject>
//================ Network Includes ======================
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>


class QAbstractManager 
	: public QObject
{
	Q_OBJECT
public:
	enum class Status { Init, WaitingForSettings, Authorization, Syncronizing, Ready, Failed };
	enum class ActionWithChanged { Push, Pull, Nothing, Other };
private:
	//===================== Private Fields ============================
	QSet<QFileInfo> localFiles, cloudFiles;
	QSet<QFileInfo> changedFiles, newLocalFiles, newCloudFiles;
	QAbstractManager::ActionWithChanged action;
	//===================== Private Methods ===========================
	void compareFiles();
protected:
	const QDir rootDir = "F:/Backups/CloudManager/";	//Hardcoded temporary
	QAbstractManager::Status status;
	//============== Pure Virtual Prorected Methods ===================
public: //debug
	virtual void downloadFile(QFileInfo) = 0;
	virtual void uploadFile(QFileInfo) = 0;
	//virtual void pushChangedFiles() = 0;
	//virtual void pullChangedFiles() = 0;
	virtual bool authorized() = 0;
	virtual void authorize() = 0;

public:
	//====================== Public Methods ===========================
	QAbstractManager();
	QAbstractManager::Status currentStatus();
	void setActionWithChanged(QAbstractManager::ActionWithChanged action);
	void syncAll();
	//============ Pure Virtual Public Methods ========================
	//virtual void init() = 0;
	//virtual qint64 spaceAvailable() = 0;		//in bites
	//virtual void downloadAllNew() = 0;
	//virtual void uploadAllNew() = 0;
	//virtual void addFile(QFileInfo) = 0;	//Pure?
	//virtual void removeFile(QFileInfo) = 0;		//Pure?
	virtual ~QAbstractManager() = 0;


	//=================== Prorected Qt Slots ==========================
protected slots:

	//======================== Qt Signals =============================
signals :
	void newFilesInTheCloud(QList<QFileInfo>);
};

//Yandex.Disk			WebDAV		REST
//Microsoft OneDrive	WebDAV
//Google Drive						REST
//Dropbox							REST













