#pragma once
#include "qobject.h"
#include "QAbstractManager.h"
#include "CommonIncludes.h"

//EXCEPTIONS
class FileHasBeenRemoved : QException {};
class FileAlreadyExists : QException {};
class Unlockable : QException {};
class IsNotLocked : QException {};


class ConfigFile :
	public QObject
{
	Q_OBJECT
	QAbstractManager* manager;
	void invalidConfig();
	void update();
	//void saveChanges();
	QJsonObject getConfigJSON();
	//TODO add device counter for removed files
	//QJsonArray files, removedFiles;
	ShortNameSet managedFilesSet;// , removedFilesSet;
	bool m_locked;
	static const ShortName configFileName; //  = ".cloudmanager";
	ShortName ConfigFile::configFileName = ".cloudmanager";
public:
	ConfigFile(QAbstractManager* manager);
	void lock();		//TODO use mutex
	void unlock();
	bool locked() const;
	void addFile(const ShortName& file);		//WARNING throws exception if file has been removed by other client device
	void removeFile(const ShortName& file);
	void removeFileData(const ShortName& file);
	ShortNameSet filesInTheCloud();
	~ConfigFile();
};

