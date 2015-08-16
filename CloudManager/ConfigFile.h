#pragma once
#include <QObject>
#include "CommonIncludes.h"
#include "FileClasses.h"

//EXCEPTIONS
class FileHasBeenRemoved : QException {};
class FileAlreadyExists : QException {};
class Unlockable : QException {};
class IsNotLocked : QException {};

class QAbstractManager;
typedef QSet<ShortName> ShortNameSet;

class ConfigFile :		//FIXME make singleton
	public QObject
{
	Q_OBJECT

	QAbstractManager* manager;
	void invalidConfig();
	void update();
	//void saveChanges();
	QJsonObject getConfigJSON();
	//TODO add device counter for removed files
	static int deviceID;
	QSet<int> defices;
	//QJsonArray files, removedFiles;
	ShortNameSet managedFilesSet;// , removedFilesSet;
	bool m_locked;
	static const ShortName configFileName; //  = ".cloudmanager";
public:
	ConfigFile(QAbstractManager*const manager);
	//ConfigFile(const ConfigFile&) = delete;
	//ConfigFile(const ConfigFile&&) = delete;
	//ConfigFile operator = (const ConfigFile&) = delete;
	//ConfigFile operator = (const ConfigFile&&) = delete;
	~ConfigFile();
	void lock();		//TODO use mutex
	void unlock();
	bool locked() const;
	void addFile(const ShortName& file);		//WARNING throws exception if file has been removed by other client device
	void removeFile(const ShortName& file);
	void removeFileData(const ShortName& file);
	ShortNameSet filesInTheCloud();
};
