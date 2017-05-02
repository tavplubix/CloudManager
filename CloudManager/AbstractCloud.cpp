#include "AbstractCloud.h"
#include "FileClasses.h"
#include "ConfigFile.h"





AbstractCloud::AbstractCloud(QString qsettingsGroup) 
	: log("network.log"), qsettingsGroup(qsettingsGroup), spaceAvailableCache(0)
{
	log.open(QIODevice::Append);
	config = nullptr;
	m_status = Status::Init;
	action = ActionWithChanged::SaveNewest;
	timer300s.setInterval(300 * 1000);
	connect(&timer300s, &QTimer::timeout, this, [&](){ if (m_status == Status::Ready) spaceAvailableCache = m_spaceAvailable(); });
}

AbstractCloud::~AbstractCloud()
{
	delete config;
	//settings->beginGroup(managerID());
	//settings->setValue("managedFiles", QVariant::fromValue(localFiles));
	//settings->endGroup();
	//delete settings;
}

void AbstractCloud::init()
{
	if (!authorized()) 
		authorize()->waitForResponseReady();
	config = new ConfigFile(this);
	spaceAvailableCache = m_spaceAvailable();
	//settings->beginGroup(managerID());
	//local = settings->value("managedFiles", QVariant::fromValue(ShortNameSet())).value<ShortNameSet>();
	//settings->endGroup();
	//сравнить файлы, время последнего изменения
	timer300s.start();
	m_status = Status::Ready;
}


AbstractCloud::Status AbstractCloud::status() const
{
	return m_status;
}

void AbstractCloud::setActionWithChanged(ActionWithChanged act)
{
	action = act;
}

QSet<LongName> AbstractCloud::managedFiles() const
{
	//auto tmp = config->filesInTheCloud();	//OPTIMIZE 1342ms
	//QSet<LongName> result;
	//for (auto i : tmp) result.insert(i);		//OPTIMIZE 800ms
	return config->filesInTheCloud();
}

QByteArray AbstractCloud::localMD5FileHash(const LongName& filename)
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



void AbstractCloud::syncAll()	
{
	auto files = config->filesInTheCloud();
	for (auto longName : files) {
		ShortName shortName = longName;
		QFileInfo localInfo = longName;
		if (!localInfo.exists())
			downloadFile(shortName, new QFile(longName));
		else if (localMD5FileHash(longName) != remoteMD5FileHash(shortName)) {
			QDateTime ltime = localInfo.lastModified();
			QDateTime rtime = lastModified(shortName);
			if (rtime < ltime) uploadFile(shortName, new QFile(longName));
			else downloadFile(shortName, new QFile(longName));
		}
		//else file has not been changed
	}

}

qint64 AbstractCloud::spaceAvailable() const
{
	//static time_t lastUpdate;
	//time_t now = time(nullptr);
	//if (now - lastUpdate > 150) {
	//	spaceAvailableCache = m_spaceAvailable();
	//	lastUpdate = now;
	//}
	return spaceAvailableCache;
}

void AbstractCloud::addFile(QFileInfo file)		//FIXME will not work in offline mode	
{
	LongName name = file.absoluteFilePath();
	config->addFile(name);
	uploadFile(name, new QFile(name));
}

void AbstractCloud::removeFile(QFileInfo file)	//FIXME will not work in offline mode    
{
	LongName name = file.absoluteFilePath();
	config->removeFile(name);
}

void AbstractCloud::removeFileData(QFileInfo file)
{
	QString filename = LongName(file.absoluteFilePath());
	QFile(filename).remove();
	config->removeFileData(filename);
}













[[deprecated("It's better to use Wireshark (with pre-master key logging)")]]
void AbstractCloud::netLog(QNetworkReply *reply, QIODevice* logDevice) const
{
	//#if NETLOG
	//static QFile log("network.log");
	//log.open(QIODevice::Append);
	QIODevice* log;
	if (logDevice) log = logDevice;
	else if (NETLOG) log = &this->log;
	else return;
	log->write("\n======================= RESPONSE =========================\n");
	int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	log->write("Status code: " + QString::number(code).toLocal8Bit() + "\n");
	QList<QByteArray> headers = reply->rawHeaderList();
	for (auto i : headers) log->write(i + ": " + reply->rawHeader(i) + "\n");
	log->write("BODY:\n");
	log->write(reply->peek(reply->bytesAvailable() < 1024 ? reply->bytesAvailable() : 1024));
	log->write("\n==========================================================\n");
	//log->flush();
	//#endif
}

[[deprecated("It's better to use Wireshark (with pre-master key logging)")]]
void AbstractCloud::netLog(const QByteArray& type, const QNetworkRequest &request, const QByteArray &body, QIODevice* logDevice) const
{
	//#if NETLOG
	//static QFile log("network.log");
	//log.open(QIODevice::Append);
	QIODevice* log;
	if (logDevice) log = logDevice;
	else if (NETLOG) log = &this->log;
	else return;
	log->write("\n======================== REQUEST =========================\n");
	log->write(type + "  " + request.url().toString().toLocal8Bit() + "\n");
	QList<QByteArray> headers = request.rawHeaderList();
	for (auto i : headers) log->write(i + ": " + request.rawHeader(i) + "\n");
	log->write("BODY:\n");
	log->write(body);
	log->write("\n==========================================================\n");
	//log->flush();
	//#endif
}

[[deprecated("Use RequestPrivate::waitForResponseReady() or RequestManager::waitForResponseReady() instead")]]
void AbstractCloud::waitFor(ReplyID replyid) const
{
	const QNetworkReply* reply = replyid;
	if (reply == nullptr) {
		qDebug() << "WARNING: reply == nullptr";
		return;
	}
	//WARNING deadlock if reply has been already destroyed
	/*static*/ QEventLoop loop;
	connect(reply, &QNetworkReply::destroyed, &loop, &QEventLoop::quit);
	loop.exec();
	///*static*/ QTimer timer;
	//connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	//timer.start(50);
	//while (!replyFinished(reply))
	//{
	//	loop.exec(QEventLoop::ExcludeUserInputEvents);
	//}
}

[[deprecated("Use RequestPrivate::waitForFinished() or RequestManager::waitForFinished() instead")]]
void AbstractCloud::waitForFinishedSignal(QNetworkReply* reply) const
{
	if (reply == nullptr) {
		qDebug() << "WARNING: reply == nullptr";
		return;
	}
	//WARNING deadlock if reply has been already finished
	/*static*/ QEventLoop loop;
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();
}












