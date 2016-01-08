#include "ConfigFile.h"
#include "AbstractCloud.h"
#include "FileClasses.h"


const ShortName ConfigFile::configFileName = ".cloudmanager";


void ConfigFile::invalidConfig()
{
	//TODO нужен нормальный обработчик
	qDebug() << "WARNING: invalid config JSON\n";
	//manager->m_status = QAbstractManager::Status::Failed;		//trowing exception is better
	throw FatalError();
}

void ConfigFile::update()
{
	if (remoteResourceCaptured) return;
	//if (m_locked) return; //WARNING
	QJsonObject configJSON = getConfigJSON();	//OPTIMIZE 509ms
	QByteArray jsonstring = QJsonDocument(configJSON).toJson();	//dbg
	if (!configJSON.contains("managedFiles") && !configJSON["managedFiles"].isArray())  invalidConfig();
	//if (!configJSON.contains("removedFiles") && !configJSON["removedFiles"].isArray())  invalidConfig();
	//QJsonArray tmp;
	//tmp = configJSON["removedFiles"].toArray();
	//ShortNameSet rmed;
	//for (auto i : tmp) rmed.insert(i.toString());
	//removedFilesSet += rmed;
	QJsonArray tmp = configJSON["managedFiles"].toArray();
	LongNameSet f;
	for (auto i : tmp) {
		QString tempstring = i.toString();	//dbg
		f.insert(i.toString());	//OPTIMIZE 830ms
	}
	managedFilesSet += f;	
	//managedFilesSet -= removedFilesSet;
}

void ConfigFile::saveChanges()
{
	QJsonArray newManagedFiles;
	for (auto i : managedFilesSet) newManagedFiles.append(QString(ShortName(i)));
	QJsonObject confObj;
	confObj.insert("managedFiles", newManagedFiles);
	QByteArray configFile = QJsonDocument(confObj).toJson();
	auto buf = new QBuffer;
	buf->setData(configFile);
	cloud->waitFor(cloud->uploadFile(configFileName, buf));
}

ConfigFile::ConfigFile(AbstractCloud* cloud)
	: cloud(cloud), remoteResourceCaptured(false), remoteResourceRequired(false), guards(0), mutex(QMutex::Recursive)
{
	uncapturingTimer.setSingleShot(true);
	uncapturingTimer.setInterval(defaultUncapturingTimeout);
	update();
}


ConfigFile::CaptureGuard ConfigFile::trycapture()
{
	uncapturingTimer.stop();
	remoteResourceRequired = true;
	if (remoteResourceCaptured) return CaptureGuard(this);
	//if (locked()) throw Unlockable();
	try{
		cloud->waitFor(cloud->lockFile(configFileName));
	}
	catch (...) {
		throw Unlockable();
	}
	remoteResourceCaptured = true;
	update();
	return CaptureGuard(this);
}

void ConfigFile::uncapture(int mtimeout)
{
	if (guards) return;
	if (!remoteResourceCaptured) return;
	remoteResourceRequired = false;
	auto uncaptureFinally = [&]() {
		if (remoteResourceRequired) return;
		if (remoteResourceCaptured) {
			remoteResourceCaptured = false;
			saveChanges();
			cloud->waitFor(cloud->unlockFile(configFileName));
		}
	};
	if (mtimeout < 0) {
		uncaptureFinally();
		return;
	}
	else {
		connect(&uncapturingTimer, &QTimer::timeout, this, uncaptureFinally);
		uncapturingTimer.start(mtimeout);
	}
}

// bool ConfigFile::locked() const
// {
// 	if (remoteResourseCaptured) return false;
// 	return manager->fileLocked(configFileName);
// }

void ConfigFile::addFile(const ShortName& file)
{
	auto guard = trycapture();
	managedFilesSet.insert(file);
}

void ConfigFile::removeFile(const ShortName& file)
{
	auto guard = trycapture();
	managedFilesSet.remove(file);
	cloud->remove(file);		
}

void ConfigFile::removeFileData(const ShortName& file)	//UNDONE removeFileData()
{
	auto guard = trycapture();

	cloud->remove(file);
	qDebug() << "removeFileData(): Not Implemented Correctly";
	//throw NotImplemented();
}

LongNameSet ConfigFile::filesInTheCloud()
{
	update();
	return managedFilesSet;
}

QJsonObject ConfigFile::getConfigJSON()
{
	ShortNameSet remote;
	QSharedPointer<QBuffer> configBuf(new QBuffer);
	try {
		cloud->waitFor(cloud->downloadFile(configFileName, configBuf));
	}
	catch (...) {
	}
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
	if (remoteResourceCaptured) {
		saveChanges();
		cloud->waitFor(cloud->unlockFile(configFileName));
	}
}

ConfigFile::AutoLock::AutoLock(ConfigFile * cf)
	: cf(cf)
{	
	
	bool ok = false;
	while (!ok) {
		try {
			guard = cf->trycapture();
			ok = true;
		}
		catch (Unlockable) {
			//QThread::msleep(200);	//is it better to run event loop?
			QEventLoop loop;
			QTimer::singleShot(200, &loop, &QEventLoop::quit);
			loop.exec();
		}
	}
}

ConfigFile::AutoLock::~AutoLock()
{
	cf->uncapture();
}


ConfigFile::CaptureGuard::CaptureGuard(ConfigFile * cf)
	: cf(cf)
{
	if (cf == nullptr) return;
	cf->mutex.lock();
	cf->guards++;
}

ConfigFile::CaptureGuard::CaptureGuard(CaptureGuard&& cg)
	: cf(cg.cf)
{
	cg.cf = nullptr;
}

ConfigFile::CaptureGuard& ConfigFile::CaptureGuard::operator=(CaptureGuard&& cg)
{
	if (cf != cg.cf) {
		cf = cg.cf;
		cg.cf = nullptr;
	}
	return *this;
}

ConfigFile::CaptureGuard::~CaptureGuard()
{
	if (cf == nullptr) return;
	cf->guards--;
	if (cf->guards == 0) cf->uncapture();
	cf->mutex.unlock();
}
