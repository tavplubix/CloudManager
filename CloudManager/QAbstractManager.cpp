#include "QAbstractManager.h"

#include "FileClasses.h"
#include "ConfigFile.h"


void QAbstractManager::netLog(QNetworkReply *reply) const
{
#if NETLOG
	//static QFile log("network.log");
	//log.open(QIODevice::Append);
	log.write("\n======================= RESPONSE =========================\n");
	int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	log.write("Status code: " + QString::number(code).toLocal8Bit() + "\n");
	QList<QByteArray> headers = reply->rawHeaderList();
	for (auto i : headers) log.write(i + ": " + reply->rawHeader(i) + "\n");
	log.write("BODY:\n");
	log.write(reply->peek(reply->bytesAvailable() < 128 ? reply->bytesAvailable():128));
	log.write("\n==========================================================\n");
	log.flush();
#endif
}

void QAbstractManager::netLog(const QByteArray& type, const QNetworkRequest &request, const QByteArray &body) const
{
#if NETLOG
	//static QFile log("network.log");
	//log.open(QIODevice::Append);
	log.write("\n======================== REQUEST =========================\n");
	log.write(type + "  " + request.url().toString().toLocal8Bit() + "\n");
	QList<QByteArray> headers = request.rawHeaderList();
	for (auto i : headers) log.write(i + ": " + request.rawHeader(i) + "\n");
	log.write("BODY:\n");
	log.write(body);
	log.write("\n==========================================================\n");
	log.flush();
#endif
}


void QAbstractManager::waitFor(ReplyID reply) const
{
	if (reply == nullptr) {
		qDebug() << "WARNING: reply == nullptr";
		return;
	}
	/*static*/ QEventLoop loop;
	/*static*/ QTimer timer;
	//connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	timer.start(10);
	while (!replyFinished(reply))
	{
		loop.exec();
	}
	//if (reply->isFinished()) return;
	//else loop.exec();
}

void QAbstractManager::registerReply(ReplyID reply) const
{
	replies.insert(reply);
}

void QAbstractManager::setReplyFinished(ReplyID reply) const
{
	replies.remove(reply);
}

bool QAbstractManager::replyFinished(ReplyID reply) const
{
	return !replies.contains(reply);
}

//void QAbstractManager::pushChangedFiles()	//WARNING добавить проверку на ошибки
//{
//	for (auto i : changedFiles)
//		uploadFile(i, new QFile(i));
//}
//
//void QAbstractManager::pullChangedFiles()	//WARNING добавить проверку на ошибки
//{
//	for (auto i : changedFiles)
//		downloadFile(i, new QFile(i));
//}



QAbstractManager::QAbstractManager() : log("network.log")
{
	log.open(QIODevice::Append);
	settings = new QSettings("tavplubix", "CloudManager");		//WARNING разным манагерам нужны разные настройки
	config = nullptr;
	m_status = Status::Init;
	action = ActionWithChanged::SaveNewest;
}


void QAbstractManager::init()
{
	if (!authorized()) 
		waitFor( authorize() );
	config = new ConfigFile(this);
	
	//settings->beginGroup(managerID());
	//local = settings->value("managedFiles", QVariant::fromValue(ShortNameSet())).value<ShortNameSet>();
	//settings->endGroup();
	//сравнить файлы, время последнего изменения
	m_status = Status::Ready;
}

QAbstractManager::Status QAbstractManager::status() const
{
	return m_status;
}

void QAbstractManager::setActionWithChanged(ActionWithChanged act)
{
	action = act;
}

QList<LongName> QAbstractManager::managedFiles() const
{
	auto tmp = config->filesInTheCloud().toList();
	QList<LongName> result;
	for (auto i : tmp) result.push_back(i);
	return result;
}

QByteArray QAbstractManager::localMD5FileHash(const LongName& filename)
{
	QCryptographicHash hash(QCryptographicHash::Md5);
	QFile file(filename);
	file.open(QIODevice::ReadOnly);
	hash.addData(&file);
	file.close();
#if DEBUG
	return hash.result().toHex();
#else
	return hash.result();
#endif
}



void QAbstractManager::syncAll()		//FIXME
{
	auto files = config->filesInTheCloud();
	for (auto remote : files) {
		LongName local = remote;
		QFileInfo localInfo = local;
		if (!localInfo.exists())
			downloadFile(remote, new QFile(local));
		else if (localMD5FileHash(local) != remoteMD5FileHash(remote)) {
			QDateTime ltime = localInfo.lastModified();
			QDateTime rtime = lastModified(remote);
			if (rtime < ltime) uploadFile(remote, new QFile(local));
			else downloadFile(remote, new QFile(local));
		}
		//else file has not been changed
	}

}
//
//void QAbstractManager::downloadAllNew()		//WARNING добавить проверку на ошибки
//{
//	for (auto i : newCloudFiles) {
//		downloadFile(i, new QFile(i));
//		localFiles.push_back(i);
//	}
//	newCloudFiles.clear();		//WARNING нельзя очищать список, если не уверены в том, что все файлы скачались
//}
//
//void QAbstractManager::uploadAllNew()	//WARNING добавить проверку на ошибки
//{
//	QString configFileName = rootDir.absoluteFilePath(".cloudmanager" + managerID());
//	QFile configFile(rootDir.absoluteFilePath(configFileName));
//	configFile.open(QIODevice::ReadWrite);
//	QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
//	QJsonArray files = config.value("managedFiles").toArray();
//	for (auto i : newLocalFiles) {
//		uploadFile(i,  new QFile(i));
//		cloudFiles.push_back(i);
//
//		files.push_back(i);
//		config.insert("managedFiles", files);
//	}
//	configFile.resize(0);
//	configFile.write(QJsonDocument(config).toJson());
//	configFile.close();
//	uploadFile(configFileName, new QFile(configFileName));
//	newCloudFiles.clear();		//WARNING нельзя очищать список, если не уверены в том, что все файлы загрузились
//}

void QAbstractManager::addFile(QFileInfo file)		//FIXME will not work in offline mode	//TODO throw exception in offline mode
{
	LongName name = file.absoluteFilePath();
	config->addFile(name);
	uploadFile(name, new QFile(name));
}

void QAbstractManager::removeFile(QFileInfo file)	//FIXME will not work in offline mode    //TODO throw exception in offline mode
{
	LongName name = file.absoluteFilePath();
	config->removeFile(name);
}

void QAbstractManager::removeFileData(QFileInfo)	//TODO implement
{
	qDebug() << "Not Implemented";
	throw NotImplemented();
}

QAbstractManager::~QAbstractManager()
{
	//settings->beginGroup(managerID());
	//settings->setValue("managedFiles", QVariant::fromValue(localFiles));
	//settings->endGroup();
	delete settings;
}
