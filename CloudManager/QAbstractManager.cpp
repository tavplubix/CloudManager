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
	log.write(reply->peek(reply->bytesAvailable() < 128 ? reply->bytesAvailable():128));
	log.write("\n==========================================================\n");
	log.flush();
}

void QAbstractManager::netLog(const QByteArray& type, const QNetworkRequest &request, const QByteArray &body) const
{
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
}


void QAbstractManager::pushChangedFiles()	//WARNING добавить проверку на ошибки
{
	for (auto i : changedFiles)
		uploadFile(i, new QFile(i));
}

void QAbstractManager::pullChangedFiles()	//WARNING добавить проверку на ошибки
{
	for (auto i : changedFiles)
		downloadFile(i, new QFile(i));
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
	QString configFileName = rootDir.absoluteFilePath(".cloudmanager" + managerID());
	this->downloadFile(configFileName, new QFile(configFileName));		//TODO use buffer
	loop.exec();
	QFile configFile(rootDir.absoluteFilePath(configFileName));
	if (configFile.size() == 0) {
		configFile.open(QIODevice::WriteOnly);
		configFile.write("{\"managedFiles\":[]}");
		configFile.close();
		this->uploadFile(configFileName, new QFile(configFileName));
	}
	configFile.open(QIODevice::ReadOnly);
	QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
	QJsonArray files = config.value("managedFiles").toArray();
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

QStringList QAbstractManager::managedFiles() const
{
	return (localFiles.toSet() + cloudFiles.toSet()).toList();
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
			if (local < cloud) downloadFile(i, new QFile(i));	//WARNING добавить проверку на ошибки
			else uploadFile(i, new QFile(i));
		}
		break;
	}
}

void QAbstractManager::downloadAllNew()		//WARNING добавить проверку на ошибки
{
	for (auto i : newCloudFiles) {
		downloadFile(i, new QFile(i));
		localFiles.push_back(i);
	}
	newCloudFiles.clear();		//WARNING нельзя очищать список, если не уверены в том, что все файлы скачались
}

void QAbstractManager::uploadAllNew()	//WARNING добавить проверку на ошибки
{
	QString configFileName = rootDir.absoluteFilePath(".cloudmanager" + managerID());
	QFile configFile(rootDir.absoluteFilePath(configFileName));
	configFile.open(QIODevice::ReadWrite);
	QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
	QJsonArray files = config.value("managedFiles").toArray();
	for (auto i : newLocalFiles) {
		uploadFile(i,  new QFile(i));
		cloudFiles.push_back(i);

		files.push_back(i);
		config.insert("managedFiles", files);
	}
	configFile.resize(0);
	configFile.write(QJsonDocument(config).toJson());
	configFile.close();
	uploadFile(configFileName, new QFile(configFileName));
	newCloudFiles.clear();		//WARNING нельзя очищать список, если не уверены в том, что все файлы загрузились
}

void QAbstractManager::addFile(QFileInfo file)
{
	QString absolute;
	if (file.isRelative()) absolute = rootDir.absoluteFilePath(file.filePath());
	else absolute = file.absoluteFilePath();
	if (localFiles.contains(absolute)) return;
	localFiles.push_back(absolute);
	uploadFile(rootDir.relativeFilePath(file.absoluteFilePath()), new QFile(rootDir.relativeFilePath(file.absoluteFilePath())));		//WARNING
	cloudFiles.push_back(absolute);
	QString configFileName = rootDir.absoluteFilePath(".cloudmanager" + managerID());
	QFile configFile(rootDir.absoluteFilePath(configFileName));
	configFile.open(QIODevice::ReadOnly);
	QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
	QJsonArray files = config.value("managedFiles").toArray();
	files.push_back(absolute);
	config.insert("managedFiles", files);
	configFile.resize(0);
	configFile.write(QJsonDocument(config).toJson());
	configFile.close();
	uploadFile(configFileName, new QFile(configFileName));
}

void QAbstractManager::removeFile(QFileInfo file)	//WARNING
{
	QString absolute;
	if (file.isRelative()) absolute = rootDir.absoluteFilePath(file.filePath());
	else absolute = file.absoluteFilePath();
	localFiles.removeAll(absolute);
	newLocalFiles.removeAll(absolute);
	newCloudFiles.removeAll(absolute);
	changedFiles.removeAll(absolute);
	remove(rootDir.relativeFilePath(absolute));
	cloudFiles.removeAll(absolute);
	QString configFileName = rootDir.absoluteFilePath(".cloudmanager" + managerID());
	QFile configFile(rootDir.absoluteFilePath(configFileName));
	configFile.open(QIODevice::ReadOnly);
	QJsonObject config = QJsonDocument::fromJson(configFile.readAll()).object();
	QJsonArray files = config.value("managedFiles").toArray();
	files.removeAt([&]() -> int {for (int i = 0; i < files.size(); i++) if (files.at(i) == absolute) return i; return -1;}());
	config.insert("managedFiles", files);
	configFile.resize(0);
	configFile.write(QJsonDocument(config).toJson());
	configFile.close();
	uploadFile(configFileName, new QFile(configFileName));
}

QAbstractManager::~QAbstractManager()
{
	settings->setValue("managedFiles", QVariant::fromValue(localFiles));
	delete settings;
}
