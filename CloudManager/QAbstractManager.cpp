#include "QAbstractManager.h"
#include "FileClasses.h"
#include "ConfigFile.h"


void QAbstractManager::netLog(QNetworkReply *reply, QIODevice* logDevice) const
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
	log->write(reply->peek(reply->bytesAvailable() < 1024 ? reply->bytesAvailable():1024));
	log->write("\n==========================================================\n");
	//log->flush();
//#endif
}

void QAbstractManager::netLog(const QByteArray& type, const QNetworkRequest &request, const QByteArray &body, QIODevice* logDevice) const
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


void QAbstractManager::waitFor(ReplyID replyid) const
{
	const QNetworkReply* reply= replyid;
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

void QAbstractManager::waitForFinishedSignal(QNetworkReply* reply) const
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
//
//void QAbstractManager::registerReply(ReplyID reply) const		//FIXME race condition
//{
//	replies.insert(reply);
//}
//
//void QAbstractManager::setReplyFinished(ReplyID reply) const	//FIXME race condition
//{
//	if (!replies.contains(reply)) {
//		qDebug() << "ERROR: Race condition detected in setReplyFinished(): Deadlock is possible in waitFor()\n";
//		//QThread::msleep(42);
//	}
//	replies.remove(reply);
//}
//
//bool QAbstractManager::replyFinished(ReplyID reply) const
//{
//	if (!replies.contains(reply)) return true;
//	else return reply->isFinished();
//}



QAbstractManager::QAbstractManager() : log("network.log")
{
	log.open(QIODevice::Append);
	settings = new QSettings("tavplubix", "CloudManager");		//FIXME разным манагерам нужны разные настройки
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



void QAbstractManager::syncAll()	
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

void QAbstractManager::addFile(QFileInfo file)		//FIXME will not work in offline mode	
{
	LongName name = file.absoluteFilePath();
	config->addFile(name);
	uploadFile(name, new QFile(name));
}

void QAbstractManager::removeFile(QFileInfo file)	//FIXME will not work in offline mode    
{
	LongName name = file.absoluteFilePath();
	config->removeFile(name);
}

void QAbstractManager::removeFileData(QFileInfo)	//UNDONE removeFileData()
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
