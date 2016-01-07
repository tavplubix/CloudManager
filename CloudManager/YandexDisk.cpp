#include "YandexDisk.h"
#include "FileClasses.h"


#include "RequestManager.h"


//#include <QNetworkProxy>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSsl>

YandexDiskWebDav::YandexDiskWebDav(QString qsettingsGroup)
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
	//connect(&disk, &QNetworkAccessManager::finished, this, &QAbstractManager::setReplyFinished);	// сигнал посылается, когда получен ответ сервера, но это не значит, что закончилась обработка ответа
	//есть смысл делать всё асинхронно
	//WARNING add SSL errors handler
	//disk.setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", 8888));
	auto tmpdir = QDir::temp().absolutePath();
	auto conf = QSslConfiguration::defaultConfiguration();
	conf.setPrivateKey(QSslKey());
	conf.setProtocol(QSsl::SslV3);
	//auto pk_pem = conf.privateKey().toPem();
	disk.connectToHostEncrypted("https://webdav.yandex.ru/");		//SLOW 39%

	
	auto pk_pem = conf.privateKey().toPem();
	QFile f("privkey.base64");
	f.open(QIODevice::WriteOnly);
	f.write(pk_pem);
	f.close();

}

YandexDiskWebDav::~YandexDiskWebDav()
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


ReplyID YandexDiskWebDav::authorize()		//TODO use QAuthorizer
{
	if (authorized()) return nullptr;
	OAuthClientInfo client(appID, clientSecret);
	QUrl authURL("https://oauth.yandex.ru/authorize?response_type=code&client_id=" + appID);
	OAuthServerInfo server(authURL, QUrl("https://oauth.yandex.ru/token"), "Yandex.Disk");
	OAuth2* oauth = new OAuth2(client, server);
	oauth->requestNewToken();
	connect(oauth, &OAuth2::tokenReady, this, [&](Token token){
		this->authorizationHeader = oauth->authtorizationHeader();
		oauth->deleteLater();
	});
	connect(oauth, &OAuth2::error, this, [&]() {
		this->authorize();		//try again
	});

	//CRUTCH
	QEventLoop loop;
	connect(oauth, &OAuth2::destroyed, &loop, &QEventLoop::quit);
	loop.exec();

	return nullptr;
}



qint64 YandexDiskWebDav::m_spaceAvailable()	const
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

void YandexDiskWebDav::setAuthType(AuthType type)
{
	//WARNING unsafe if any replies are running
	//if (!allReplies().isEmpty()) throw Error();
	authorizationHeader.clear();
	authtype = type;
	authorize();
}

bool YandexDiskWebDav::_checkForHTTPErrors(QNetworkReply *reply, const char* file, const int line, const char* func) const
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

void YandexDiskWebDav::createDirIfNecessary(ShortName dirname)
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

bool YandexDiskWebDav::authorized() const
{
	if (authorizationHeader.isEmpty()) return false;
	//if (validUntil - time(nullptr) < 600) return false;
	try {
		m_spaceAvailable();	//throws if not authorized
		return true;
	} catch (NotAuthorized) {
		return false;
	}
}



ReplyID YandexDiskWebDav::downloadFile(const ShortName& name, QSharedPointer<QIODevice> file)
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


ReplyID YandexDiskWebDav::uploadFile(const ShortName& name, QIODevice* file)
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
	spaceAvailableCache -= file->size();
	return reply;
}

QDateTime YandexDiskWebDav::lastModified(const ShortName& name) const
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

QByteArray YandexDiskWebDav::remoteMD5FileHash(const ShortName& filename) const
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


QString YandexDiskWebDav::userName() const
{
	QNetworkRequest request(QUrl("https://webdav.yandex.ru/?userinfo"));
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply* reply = disk.get(request);
	netLog("MKCOL", request, "EMPTY");
	waitForFinishedSignal(reply);
	checkForHTTPErrors(reply);
	QByteArray body = reply->readAll();
	int begin = body.indexOf("login:");
	int end = body.indexOf('\n', begin);
	if (begin < 0) return "unknown";
	else {
		begin += qstrlen("login:");
		return body.mid(begin, end - begin);
	}
}

void YandexDiskWebDav::mkdir(ShortName dir)		//TODO add DirName class
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

ReplyID YandexDiskWebDav::remove(const ShortName& name)
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
	spaceAvailableCache += QFileInfo(LongName(name)).size();
	return reply;
	//TODO remove dir if empty
}


QByteArray YandexDiskWebDav::sendDebugRequest(QByteArray requestType, QString url, QByteArray body, QList<QNetworkReply::RawHeaderPair> additionalHeaders)
{
	RequestManager rm(url);
	QNetworkRequest request(url);
	for (auto &header : additionalHeaders)
		request.setRawHeader(header.first, header.second);
	if (!request.hasRawHeader("Authorization"))
		request.setRawHeader("Authorization", authorizationHeader);

	QByteArray log;
	log += "\n======================== REQUEST =========================\n";
	log += requestType + "  " + request.url().toString().toLocal8Bit() + "\n";
	QList<QByteArray> headers = request.rawHeaderList();
	for (auto i : headers) log += i + ": " + request.rawHeader(i) + "\n";
	log += "BODY:\n";
	log += body;
	log += "\n==========================================================\n";

	RequestID reqID;
	requestType = requestType.toUpper();
	if (requestType == "HEAD")			reqID = rm.HEAD(request);	//CRUTCH this crutch makes it works
	else if (requestType == "GET")		reqID = rm.GET(request);
	else if (requestType == "PUT")		reqID = rm.PUT(request, body);
	else if (requestType == "POST")		reqID = rm.POST(request, body);
	else if (requestType == "DELETE")	reqID = rm.DELETE(request);
	else {
		return QByteArray();
	}
	
	rm.waitForResponseReady(reqID);

	Request* req = rm.getRequestByID(reqID);
	log += "\n======================= RESPONSE =========================\n";
	int code = req->httpStatusCode();
	log += "Status code: " + QString::number(code).toLocal8Bit() + "\n";
	auto respheaders = req->responseHeaders();
	for (auto i : respheaders) log += (i.first + ": " + i.second + "\n");
	log += "BODY:\n";
	log += req->responseBody().left(1024);

	return log;


	/*
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
	*/
}

