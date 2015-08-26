#include "YandexDisk.h"
#include "FileClasses.h"


YandexDisk::YandexDisk(QString qsettingsGroup)
	: AbstractCloud(qsettingsGroup)
{
	oauth = nullptr;
	m_status = Status::Init;
	//restore settings
	if (!qsettingsGroup.isEmpty()) {
		QSettings settings;
		settings.beginGroup(qsettingsGroup);
		authtype = static_cast<AuthType>(settings.value("aythtype", static_cast<int>(AuthType::OAuth)).toInt());		// I really love static typing
		authorizationHeader = settings.value("authorization").value<QByteArray>();	//TODO encrypt token
		settings.endGroup();
	}
	//connect(&disk, &QNetworkAccessManager::finished, this, &QAbstractManager::setReplyFinished);	//FIXME сигнал посылается, когда получен ответ сервера, но это не значит, что закончилась обработка ответа
	//есть смысл делать всё асинхронно
	disk.connectToHostEncrypted("https://webdav.yandex.ru/");	//WARNING add SSL errors handler
}

YandexDisk::~YandexDisk()
{
	//save settings
	if (!qsettingsGroup.isEmpty()) {
		QSettings settings;
		settings.beginGroup(qsettingsGroup);
		settings.setValue("authtype", static_cast<int>(authtype));
		settings.setValue("authorization", QVariant::fromValue(authorizationHeader));
		settings.endGroup();
	}
	//TODO close all connections correctly
}


ReplyID YandexDisk::authorize()		//TODO use QAuthorizer
{
	if (authorized()) return nullptr;
	if (authtype == AuthType::Basic) {
		qDebug() << "Basic authorization isn't implemented\n";
		authtype = AuthType::OAuth;
	}
	if (authtype == AuthType::OAuth) {
		//Получение кода верификации
		QString authURL = "https://oauth.yandex.ru/authorize?response_type=code";
		authURL += "&client_id=" + appID;
		//authURL += "&device_id=" + deviceID;
		//authURL += "&device_name=" + deviceName;
		QString label = QString::fromLocal8Bit("Введите код верификации. Его можно получить <a href=");
		label += "\"" + authURL + "\"" + QString::fromLocal8Bit(">здесь</a>.\n");
		int verificationCode = QInputDialog::getInt(qApp->activeWindow(), QString::fromLocal8Bit("Авторизация"), label, -1, 0);
		if (verificationCode < 0) {
			m_status = Status::Failed;
			return nullptr;
		}

		//Запрос токена
		if (oauth == nullptr) {
			oauth = new QNetworkAccessManager(this);
			//connect(oauth, &QNetworkAccessManager::finished, this, &QAbstractManager::setReplyFinished);
		}
		QUrl authServer = authURL;
		oauth->connectToHostEncrypted("https://oauth.yandex.ru");	//WARNING add SSL errors handler
		QByteArray requestBody = "grant_type=authorization_code";
		requestBody += "&code=" + QString::number(verificationCode);
		requestBody += "&client_id=" + appID;
		requestBody += "&client_secret=" + clientSecret;
		//requestBody += "&device_id=" + deviceID;
		//requestBody += "&device_name=" + deviceName;
		QNetworkReply *reply = oauth->post(QNetworkRequest(QUrl("https://oauth.yandex.ru/token")), QByteArray(requestBody));
		//		registerReply(reply);
		netLog("POST", QNetworkRequest(QUrl("https://oauth.yandex.ru/token")), requestBody);	//DEBUG
		//connect(reply, &QNetworkReply::finished, this, &YandexDiskManager::tokenGot);
		connect(reply, &QNetworkReply::finished, this, [=]() {
			bool ok = false;
			checkForHTTPErrors(reply);
			QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
			reply->deleteLater();
			if (response.contains("access_token")) {
				authorizationHeader = "OAuth " + response["access_token"].toString().toLocal8Bit();
				validUntil = time(nullptr) + response["expires_in"].toInt();
				ok = true;
			}
			else {
				qDebug() << "Token request: Error: " << response["error_description"].toString() << " Code: " << response["error"].toString();
			}
			if (ok) {
				delete oauth;
				oauth = nullptr;
				m_status = Status::Ready;
				//emit done();
			}
			else {
				//QTimer::singleShot(1000, this, &YandexDiskManager::authorize);
				throw NotAuthorized();
			}
		});
		return reply;
	}
	return nullptr;
}



qint64 YandexDisk::spaceAvailable()	const
//варианты:
// 1 заблокировать поток, отправить запрос в другом потоке (Блокировка потока - плохо)
// 2 добавить функцию updateAvailableSpace(), из этой ф-ции возвращать неактуальные данные
// 3 возвращать значение сигналом (хуйня?)
// 4 осилить XMPP (второй вариант + получать уведомления об изменении доступного объёма, инфа актуальна) 
// xmpp _СЛИШКОМ_ ебаный, лучше ВРЕМЕННО закостылять первым способом
{

	QNetworkRequest request(QUrl("https://webdav.yandex.ru/"));
	request.setRawHeader("AUthorization", authorizationHeader);
	request.setRawHeader("Depth", "0");
	QByteArray body = "<propfind xmlns=\"DAV:\"> <prop> <quota-available-bytes/> <quota-used-bytes/> </prop> </propfind>";
	QBuffer *buf = new QBuffer;
	buf->setData(body);
	buf->open(QIODevice::ReadOnly);
	QNetworkReply *reply = disk.sendCustomRequest(request, "PROPFIND", buf);
	//	registerReply(reply);
	netLog("PROPFIND", request, body);
	buf->setParent(reply);
	waitForFinishedSignal(reply);

	checkForHTTPErrors(reply);
	if (HTTPstatus(reply) == 401) throw NotAuthorized();
	QByteArray response = reply->readAll();
	reply->deleteLater();
	response.toLower();
	int begin = response.indexOf("<d:quota-available-bytes>");		//CRUTCH using XML parser is better
	begin += QByteArray("<d:quota-available-bytes>").length();
	int end = response.indexOf("</d:quota-available-bytes>");
	QByteArray bytes = response.mid(begin, end - begin);
	return bytes.toLongLong();
	/*QDomDocument xml(reply->readAll());
	reply->deleteLater();
	QDomNodeList temp = xml.elementsByTagName("d:quota-available-bytes");
	if (temp.isEmpty()) return -1;
	return temp.at(0).toElement().text().toInt();*/
}

void YandexDisk::setAuthType(AuthType type)
{
	//WARNING unsafe if any replies are running
	//if (!allReplies().isEmpty()) throw Error();
	authorizationHeader.clear();
	authtype = type;
	authorize();
}

bool YandexDisk::_checkForHTTPErrors(QNetworkReply *reply, const char* file, const int line, const char* func) const
{
	netLog(reply);  //debug
	int code = HTTPstatus(reply);
	int codeType = (code / 100) * 100;
	if (codeType == 400 || codeType == 500) {
		QMessageLogger(file, line, func).debug() << "Bad HTTP response: Status code: " << code;
		//qDebug() << "Bad HTTP response: Status code: " << statusCode;
		return true;
	}
	else return false;
}

void YandexDisk::createDirIfNecessary(ShortName dirname)
{
	if (dirname == ".") return;
	QNetworkRequest request("https://webdav.yandex.ru/" + dirname);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply* reply = disk.head(request);
	//	registerReply(reply);
	waitForFinishedSignal(reply);
	int status = HTTPstatus(reply);
	if (HTTPstatus(reply) == 404) {
		QFileInfo up(dirname);
		auto tmp = up.path();
		//tmp = up.path();
		createDirIfNecessary(up.path());
		try {
			mkdir(dirname);
		} catch (ItsNotDir) {
			throw InvalidPath();
		}
	}
	delete reply;
}

bool YandexDisk::authorized() const
{
	if (authorizationHeader.isEmpty()) return false;
	//if (validUntil - time(nullptr) < 600) return false;
	try {
		spaceAvailable();	//throws if not authorized
		return true;
	} catch (NotAuthorized) {
		return false;
	}
}



ReplyID YandexDisk::downloadFile(const ShortName& name, QSharedPointer<QIODevice> file)
{
	QNetworkRequest request("https://webdav.yandex.ru/" + name);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.get(request);
	//	registerReply(reply);
	netLog("GET", request, "EMPTY");	//DEBUG
	connect(reply, &QNetworkReply::finished, this, [=](){
		bool ok = false;
		checkForHTTPErrors(reply);
		QByteArray MD5hash = reply->rawHeader("Etag");
		file->open(QIODevice::WriteOnly);
		file->write(reply->readAll());
		file->close();
		//check md5
		QCryptographicHash hashCalculator(QCryptographicHash::Algorithm::Md5);
		file->open(QIODevice::ReadOnly);
		hashCalculator.addData(file.data());
		file->close();
		QByteArray fileHash = hashCalculator.result().toHex();
		if (fileHash == MD5hash) ok = true;
		if (!ok) qDebug() << QString::fromLocal8Bit("Хуйня");
		reply->deleteLater();
		//emit done();
	});
	return reply;
}


ReplyID YandexDisk::uploadFile(const ShortName& name, QIODevice* file)
{
	createDirIfNecessary(QFileInfo(name).path());
	QNetworkRequest request("https://webdav.yandex.ru/" + name);
	request.setRawHeader("Authorization", authorizationHeader);
	file->open(QIODevice::ReadOnly);
	QCryptographicHash md5HashCalculator(QCryptographicHash::Algorithm::Md5);
	md5HashCalculator.addData(file);
	request.setRawHeader("Etag", md5HashCalculator.result().toHex());
	file->reset();
	QCryptographicHash sha256HashCalculator(QCryptographicHash::Algorithm::Sha256);
	sha256HashCalculator.addData(file);
	request.setRawHeader("Sha256", sha256HashCalculator.result().toHex());
	file->reset();
	QNetworkReply *reply = disk.put(request, file);
	//	registerReply(reply);
	file->setParent(reply);
	netLog("PUT", request, "FILE");
	connect(reply, &QNetworkReply::finished, this, [=](){
		checkForHTTPErrors(reply);
		int statusCode = HTTPstatus(reply);
		if (statusCode == 100) return;
		if (statusCode == 201) reply->abort();
		reply->deleteLater();
	});
	return reply;
}

QDateTime YandexDisk::lastModified(const ShortName& name) const
{
	QNetworkRequest request("https://webdav.yandex.ru/" + name);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.head(request);
	//	registerReply(reply);
	netLog("HEAD", request, "EMPTY");
	//connect(reply, &QNetworkReply::finished, this, [&](){ setReplyFinished(reply); });		//CRUTCH
	waitForFinishedSignal(reply);
	checkForHTTPErrors(reply);
	QByteArray time = reply->rawHeader("Last-Modified");
	delete reply;
	if (time.isEmpty()) return QDateTime::fromTime_t(0);
	return QDateTime::fromString(time, Qt::RFC2822Date);
}

QByteArray YandexDisk::remoteMD5FileHash(const ShortName& filename) const
{
	QNetworkRequest request("https://webdav.yandex.ru/" + filename);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.head(request);
	//	registerReply(reply);
	netLog("HEAD", request, "EMPTY");
	//connect(reply, &QNetworkReply::finished, this, [&](){ setReplyFinished(reply); });		//CRUTCH
	waitForFinishedSignal(reply);
	checkForHTTPErrors(reply);
	QByteArray hash = reply->rawHeader("Etag");
	delete reply;
#if DEBUG
	return hash;
#else
	return QByteArray::fromHex(hash);
#endif
}


QString YandexDisk::userName()
{
	QNetworkRequest request(QUrl("https://webdav.yandex.ru/?userinfo"));
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply* reply = disk.get(request);
	netLog("MKCOL", request, "EMPTY");
	waitForFinishedSignal(reply);
	checkForHTTPErrors(reply);
	QByteArray body = reply->readAll();
	int begin = body.indexOf("login:");
	int end = body.indexOf('\n');
	if (begin < 0) return "unknown";
	else return body.mid(begin + qstrlen("login:"), ++end ? end : -1);
}

void YandexDisk::mkdir(ShortName dir)		//TODO add DirName class
{
	QNetworkRequest request("https://webdav.yandex.ru/" + dir/*relative*/);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.sendCustomRequest(request, "MKCOL");
	//	registerReply(reply);
	netLog("MKCOL", request, "EMPTY");
	waitForFinishedSignal(reply);

	checkForHTTPErrors(reply);
	if (HTTPstatus(reply) == 405) {		//if resource already exists
		//check if this resource is a file instead of a dir
		QNetworkRequest request("https://webdav.yandex.ru/" + dir);
		request.setRawHeader("Authorization", authorizationHeader);
		QNetworkReply* reply = disk.head(request);		//TODO using PROPFIND is better
		//		registerReply(reply);
		waitForFinishedSignal(reply);
		checkForHTTPErrors(reply);
		if (reply->hasRawHeader("Etag")) throw ItsNotDir();	//dirs don't have MD5-hash 
	}
	delete reply;
	//emit done();

}

ReplyID YandexDisk::remove(const ShortName& name)
{
	QNetworkRequest request("https://webdav.yandex.ru/" + name);
	request.setRawHeader("Authorization", authorizationHeader);
	//QNetworkReply *reply = disk.sendCustomRequest(request, "DELETE");
	QNetworkReply *reply = disk.deleteResource(request);
	//	registerReply(reply);
	netLog("DELETE", request, "EMPTY");	//DEBUG
	connect(reply, &QNetworkReply::finished, this, [=](){
		checkForHTTPErrors(reply);
		reply->deleteLater();
		//emit done();
	});
	return reply;
	//TODO remove dir if empty
}


QByteArray YandexDisk::sendDebugRequest(QByteArray requestType, QString url, QByteArray body, QList<QNetworkReply::RawHeaderPair> additionalHeaders)
{
	QNetworkRequest request(url);
	for (auto &header : additionalHeaders)
		request.setRawHeader(header.first, header.second);
	if (!request.hasRawHeader("Authorization"))
		request.setRawHeader("Authorization", authorizationHeader);

	QNetworkReply* reply;
	requestType = requestType.toUpper();
	if (requestType == "HEAD")		reply = disk.head(request);	//CRUTCH this crutch makes it works
	else if (requestType == "GET")		reply = disk.get(request);
	else if (requestType == "PUT")		reply = disk.put(request, body);
	else if (requestType == "POST")		reply = disk.post(request, body);
	else if (requestType == "DELETE")	reply = disk.deleteResource(request);
	else {
		QBuffer *buf = new QBuffer;
		buf->setData(body);
		buf->open(QIODevice::ReadOnly);
		reply = disk.sendCustomRequest(request, requestType, buf);
		buf->setParent(reply);
	}
	//	registerReply(reply);

	QBuffer logBuf;
	logBuf.open(QIODevice::Append);
	netLog(requestType, request, body, &logBuf);
	waitForFinishedSignal(reply);
	netLog(reply, &logBuf);
	delete reply;
	return logBuf.data();
}

