#include "QAbstractManager.h"


void QAbstractManager::netLog(QNetworkReply *reply) const
{
	//static QFile log("network.log");
	//log.open(QIODevice::Append);
	log.write("\n======================= RESPONSE =========================\n");
	int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	log.write("Status code: " + QString::number(code).toLocal8Bit() + "\n");
	QList<QByteArray> headers = reply->rawHeaderList();
	for (auto i : headers) log.write(i + ": " + reply->rawHeader(i) + "\n");
	log.write("BODY:\n");
	log.write(reply->peek(2048));
	log.write("\n==========================================================\n");
	log.flush();
}

void QAbstractManager::netLog(const QNetworkRequest &request, const QByteArray &body) const
{
	//static QFile log("network.log");
	//log.open(QIODevice::Append);
	log.write("\n======================== REQUEST =========================\n");
	log.write("Requested URL: " + request.url().toString().toLocal8Bit() + "\n");
	QList<QByteArray> headers = request.rawHeaderList();
	for (auto i : headers) log.write(i + ": " + request.rawHeader(i) + "\n");
	log.write("BODY:\n");
	log.write(body);
	log.write("\n==========================================================\n");
	log.flush();
}


void QAbstractManager::pushChangedFiles()	//WARNING добавить проверку на ошибки
{
	for (auto i : changedFiles)
		uploadFile(i);
}

void QAbstractManager::pullChangedFiles()	//WARNING добавить проверку на ошибки
{
	for (auto i : changedFiles)
		downloadFile(i);
}

/*QSslConfiguration QAbstractManager::sslconf_DEBUG_ONLY()
{
	const QString privateKeyFile = "key.pem";
	//const QByteArray password = "wireshark";
	QFile f(privateKeyFile);
	f.open(QIODevice::ReadOnly);
	QByteArray k = f.readAll();
	QSslKey key(k, QSsl::KeyAlgorithm::Rsa, QSsl::EncodingFormat::Pem, QSsl::KeyType::PrivateKey);
	if (key.isNull()) throw "xyu";
	QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
	conf.setPrivateKey(key);
	if (conf.isNull()) throw "xyu";
	return conf;
}*/

QAbstractManager::QAbstractManager() : log("network.log")
{
	log.open(QIODevice::Append);
	settings = new QSettings("tavplubix", "CloudManager");
	m_status = Status::Init;
	action = ActionWithChanged::SaveNewest;
}


void QAbstractManager::init()
{
	QEventLoop loop;
	connect(this, &QAbstractManager::done, &loop, &QEventLoop::quit);
	if (!this->authorized()) {
		this->authorize();
		loop.exec();
	}
	QString configFileName = ".cloudmanager" + managerID();
	this->downloadFile(configFileName);
	loop.exec();
	QFile configFile(rootDir.absoluteFilePath(configFileName));
	if (configFile.size() == 0) {
		configFile.open(QIODevice::WriteOnly);
		configFile.write("{\"managedFiles\":[]}");
		configFile.close();
		this->uploadFile(configFileName);
	}
	configFile.open(QIODevice::ReadOnly);
	QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
	QJsonArray files = config.take("managedFiles").toArray();
	for (auto i : files) 
		cloudFiles.push_back(i.toString());
	localFiles = settings->value("managedFiles", QVariant::fromValue(QStringList())).value<QStringList>();
	newLocalFiles = (localFiles.toSet() - cloudFiles.toSet() ).toList();			//OPTIMIZE
	newCloudFiles = (cloudFiles.toSet() - localFiles.toSet() ).toList();
	auto oldFiles = (localFiles.toSet() & cloudFiles.toSet()).toList();
	//сравнить файлы, время последнего изменения
	for (auto i : oldFiles) {		//OPTIMIZE saveNewest
		auto local = QFileInfo(i).lastModified();
		auto cloud = lastModified(i);
		if (local != cloud) changedFiles.push_back(i);		
	}
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

void QAbstractManager::syncAll()
{
	downloadAllNew();		//WARNING
	uploadAllNew();			//WARNING
	switch (action) {
	case ActionWithChanged::Pull :
		pullChangedFiles();
		break;
	case ActionWithChanged::Push :
		pushChangedFiles();
		break;
	case ActionWithChanged::SaveNewest :
		for (auto i : changedFiles) {		//OPTIMIZE saveNewest
			auto local = QFileInfo(i).lastModified();
			auto cloud = lastModified(i);
			if (local < cloud) downloadFile(i);	//WARNING добавить проверку на ошибки
			else uploadFile(i);
		}
		break;
	}
}

void QAbstractManager::downloadAllNew()		//WARNING добавить проверку на ошибки
{
	for (auto i : newCloudFiles) {
		downloadFile(i);
		localFiles.push_back(i);
	}
	newCloudFiles.clear();		//WARNING нельзя очищать список, если не уверены в том, что все файлы скачались
}

void QAbstractManager::uploadAllNew()	//WARNING добавить проверку на ошибки
{
	for (auto i : newLocalFiles) {
		uploadFile(i);
		cloudFiles.push_back(i);
	}
	newCloudFiles.clear();		//WARNING нельзя очищать список, если не уверены в том, что все файлы загрузились
}

void QAbstractManager::addFile(QFileInfo file)
{
	QString absolute;
	if (file.isRelative()) absolute = rootDir.relativeFilePath(file.filePath());
	else absolute = file.absoluteFilePath();
	localFiles.push_back(absolute);
	uploadFile(file);
	cloudFiles.push_back(absolute);
}

void QAbstractManager::removeFile(QFileInfo file)	//WARNING
{
	QString absolute;
	if (file.isRelative()) absolute = rootDir.relativeFilePath(file.filePath());
	else absolute = file.absoluteFilePath();
	localFiles.removeAll(absolute);
	newLocalFiles.removeAll(absolute);
	newCloudFiles.removeAll(absolute);
	changedFiles.removeAll(absolute);
	remove(absolute);
	cloudFiles.removeAll(absolute);
}

QAbstractManager::~QAbstractManager()
{
	settings->setValue("managedFiles", QVariant::fromValue(localFiles));
	delete settings;
}
