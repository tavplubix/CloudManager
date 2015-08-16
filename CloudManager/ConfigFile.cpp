#include "ConfigFile.h"


void ConfigFile::invalidConfig()
{
	//TODO нужен нормальный обработчик
	qDebug() << "WARNING: invalid config JSON\n";
	//manager->m_status = QAbstractManager::Status::Failed;		//trowing exception is better
	throw FatalError();
}

void ConfigFile::update()
{
	//if (m_locked) return; //WARNING
	QJsonObject configJSON = getConfigJSON();
	if (!configJSON.contains("managedFiles") && !configJSON["managedFiles"].isArray())  invalidConfig();
	//if (!configJSON.contains("removedFiles") && !configJSON["removedFiles"].isArray())  invalidConfig();
	QJsonArray tmp;
	//tmp = configJSON["removedFiles"].toArray();
	//ShortNameSet rmed;
	//for (auto i : tmp) rmed.insert(i.toString());
	//removedFilesSet += rmed;
	tmp = configJSON["managedFiles"].toArray();
	ShortNameSet f;
	for (auto i : tmp) f.insert(i.toString());
	managedFilesSet += f;	
	//managedFilesSet -= removedFilesSet;
}
//
//void ConfigFile::saveChanges()
//{
//	bool localLock = !locked();		//is it necessary?
//	if (localLock) lock();
//	update();
//	QJsonArray newManagedFiles;
//	for (auto i : files) newManagedFiles.append(i);
//	files = newManagedFiles;
//	QJsonArray newRemovedFiles;
//	for (auto i : files) newRemovedFiles.append(i);
//	removedFiles = newRemovedFiles;
//	QJsonObject confObj;
//	confObj.insert("managedFiles", files);
//	confObj.insert("removedFiles", removedFiles);
//	QByteArray configFile = QJsonDocument(confObj).toJson();
//	auto buf = new QBuffer;
//	buf->setData(configFile);
//	manager->waitFor( manager->uploadFile(configFileName, buf) );
//	if (localLock) unlock();
//}

ConfigFile::ConfigFile(QAbstractManager* manager)
	: manager(manager)
{
	update();
}


void ConfigFile::lock()
{
	if (locked()) throw Unlockable();
	try{
		manager->waitFor( manager->lockFile(configFileName) );
	}
	catch (...) {
		throw Unlockable();
	}
	m_locked = true;
	update();
}

void ConfigFile::unlock()
{
	if (!m_locked) return;
	//saveChanges();
	manager->waitFor( manager->unlockFile(configFileName) );
}

bool ConfigFile::locked() const
{
	if (m_locked) return true;
	return manager->fileLocked(configFileName);
}

void ConfigFile::addFile(const ShortName& file)
{
	lock();
	managedFilesSet.insert(file);
	QJsonArray newManagedFiles;
	for (auto i : managedFilesSet) newManagedFiles.append((QString)i);
	QJsonObject confObj;
	confObj.insert("managedFiles", newManagedFiles);
	QByteArray configFile = QJsonDocument(confObj).toJson();
	auto buf = new QBuffer;
	buf->setData(configFile);
	manager->waitFor( manager->uploadFile(configFileName, buf) );
	unlock();
}

void ConfigFile::removeFile(const ShortName& file)
{
	lock();
	managedFilesSet.remove(file);
	QJsonArray newManagedFiles;
	for (auto i : managedFilesSet) newManagedFiles.append((QString)i);
	QJsonObject confObj;
	confObj.insert("managedFiles", newManagedFiles);
	QByteArray configFile = QJsonDocument(confObj).toJson();
	auto buf = new QBuffer;
	buf->setData(configFile);
	manager->waitFor(manager->uploadFile(configFileName, buf));
	unlock();
	manager->remove(file);		//WARNING 
}

void ConfigFile::removeFileData(const ShortName& file)
{
	qDebug() << "Not Implemented";
	throw NotImplemented();
}

ShortNameSet ConfigFile::filesInTheCloud() 
{
	update();
	return managedFilesSet;
}

QJsonObject ConfigFile::getConfigJSON()
{
	ShortNameSet remote;
	QSharedPointer<QBuffer> configBuf(new QBuffer);
	manager->waitFor(manager->downloadFile(configFileName, configBuf));
	if (configBuf->bytesAvailable() == 0) {
		//TODO нужна отдельная функция для случая, когда манагер запущен впервые
		qDebug() << "WARNING: config JSON is empty\n";
		configBuf->setData("{\"managedFiles\":[], \"removedFiles\":[]}", qstrlen("{\"managedFiles\":[], \"removedFiles\":[]}"));
	}
	QJsonDocument configJSONdoc = QJsonDocument::fromJson(configBuf->data());
	if (configJSONdoc.isNull()) 
		invalidConfig();
	return configJSONdoc.object();
}

ConfigFile::~ConfigFile()
{
	if (locked()) unlock();
}
