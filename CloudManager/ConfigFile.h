#pragma once
#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "CommonIncludes.h"
#include "FileClasses.h"

//EXCEPTIONS
class FileHasBeenRemoved : QException {};
class FileAlreadyExists : QException {};
class Unlockable : QException {};
class IsNotLocked : QException {};

class AbstractCloud;
typedef QSet<ShortName> ShortNameSet;

class ConfigFile :		//FIXME make singleton
	public QObject
{
	Q_OBJECT
	friend class AutoLock;
	friend class CaptureGuard;
	int guards;		//CRUTCH
	QMutex mutex;	//FIXME mutex
	class CaptureGuard {	//TODO add a counter of a references
		friend class ConfigFile;
		ConfigFile * cf;
		CaptureGuard(ConfigFile * cf = nullptr);
		CaptureGuard(const CaptureGuard&) = delete;
		CaptureGuard& operator = (const CaptureGuard&) = delete;	
	public:
		CaptureGuard(CaptureGuard&&);
		CaptureGuard& operator = (CaptureGuard&&);
		~CaptureGuard();
	};
	AbstractCloud* cloud;
	void invalidConfig();
	void update();
	void saveChanges(); 
	QJsonObject getConfigJSON();
	//bool locked() const;
	QTimer uncapturingTimer;
	//enum ConfigFileState {Unknown, Free, Locked, Required, Captured, Uncapturing, PostUncapturing};
	///*volatile*/ ConfigFileState state;
	static const int defaultUncapturingTimeout = 5000;	//msecs
	/*volatile*/ bool remoteResourceCaptured;
	/*volatile*/ bool remoteResourceRequired;	//TODO use QObject::blockSignals() for QTimer instead
	CaptureGuard trycapture();
	void uncapture(int mtimeout = defaultUncapturingTimeout);	//msesc
	//TODO add device counter for removed files
	static int deviceID;
	QSet<int> defices;
	//QJsonArray files, removedFiles;
	LongNameSet managedFilesSet;// , removedFilesSet;
	//bool m_locked;
	static const ShortName configFileName; //  = ".cloudmanager";
public:
	ConfigFile(AbstractCloud*const cloud);
	ConfigFile(const ConfigFile&) = delete;
	ConfigFile(const ConfigFile&&) = delete;
	ConfigFile operator = (const ConfigFile&) = delete;
	ConfigFile operator = (const ConfigFile&&) = delete;
	~ConfigFile();
	void addFile(const ShortName& file);		//WARNING throws exception if file has been removed by other client device
	void removeFile(const ShortName& file);
	void removeFileData(const ShortName& file);
	LongNameSet filesInTheCloud();
	class AutoLock {
		ConfigFile * const cf;
		ConfigFile::CaptureGuard guard;
	public:
		AutoLock(ConfigFile * cf);
		~AutoLock();
		AutoLock(const AutoLock&) = delete;
		AutoLock(const AutoLock&&) = delete;
		AutoLock operator = (const AutoLock&) = delete;
		AutoLock operator = (const AutoLock&&) = delete;
	};
};
