#include "YandexDiskREST.h"








bool YandexDiskREST::authorized() const
{
	try {
		userName();
	}
	catch (UnexpectedHttpStatusCode* e) {
		if (e->statusCode() == 401)
			return false;
		qDebug() << "HTTP STATUS CODE: " << e->statusCode();
		throw;
	}
	return true;
}

ReplyID YandexDiskREST::authorize()
{
	if (authorized()) return nullptr;
	OAuthClientInfo client(appID, clientSecret);
	QUrl authURL("https://oauth.yandex.ru/authorize?response_type=code&client_id=" + appID);
	OAuthServerInfo server(authURL, QUrl("https://oauth.yandex.ru/token"), "Yandex.Disk");
	OAuth2* oauth = new OAuth2(client, server);
	oauth->requestNewToken();
	connect(oauth, &OAuth2::tokenReady, this, [&](Token token) {
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



qint64 YandexDiskREST::m_spaceAvailable() const
{
	QNetworkRequest req(apiURL);
	req.setRawHeader("Authorization", authorizationHeader);
	RequestID id = api.GET(req);
	api.waitForResponseReady(id);
	Request* r = api.getRequestByID(id);
	if (r->httpStatusCode() != 200)
		throw new UnexpectedHttpStatusCode(r->httpStatusCode());
	QJsonObject diskInfo = QJsonDocument::fromJson(r->responseBody()).object();
	qint64 total = diskInfo["total_space"].toInt();		//WARNING
	qint64 used = diskInfo["used_space"].toInt();
	return total - used;
}



ReplyID YandexDiskREST::downloadFile(const ShortName& name, QSharedPointer<QIODevice> file)
{
	QNetworkRequest req(apiURL + "resources/download?path=" + name);
	req.setRawHeader("Authorization", authorizationHeader);
	RequestID id = api.GET(req);
	api.waitForResponseReady(id);
	Request* r = api.getRequestByID(id);
	if (r->httpStatusCode() != 200)
		throw new UnexpectedHttpStatusCode(r->httpStatusCode());
	QJsonObject downloadingInfo = QJsonDocument::fromJson(r->responseBody()).object();
	QUrl href = downloadingInfo["href"].toString();

	id = disk.GET(QNetworkRequest(href));
	auto dr = disk.getRequestByID(id);
	connect(dr, &Request::responseReady, [&]() {
		file->open(QIODevice::WriteOnly);
		file->write(dr->responseBody());
	});

	return disk.__CRUTCH__get_QNetworkReply_object_by_RequestID__(id);
}

ReplyID YandexDiskREST::uploadFile(const ShortName& name, QIODevice* file)
{
	QNetworkRequest req(apiURL + "resources/upload?overwrite=true&path=" + name);
	req.setRawHeader("Authorization", authorizationHeader);
	RequestID id = api.GET(req);
	api.waitForResponseReady(id);
	Request* r = api.getRequestByID(id);
	if (r->httpStatusCode() != 200)
		throw new UnexpectedHttpStatusCode(r->httpStatusCode());
	QJsonObject uploadingInfo = QJsonDocument::fromJson(r->responseBody()).object();
	QUrl href = uploadingInfo["href"].toString();

	id = disk.PUT(QNetworkRequest(href), file);
	return disk.__CRUTCH__get_QNetworkReply_object_by_RequestID__(id);
}



ReplyID YandexDiskREST::remove(const ShortName& name)
{
	QNetworkRequest req(apiURL + "resources?permanently=true&path=" + name);
	req.setRawHeader("Authorization", authorizationHeader);
	RequestID id = api.GET(req);
	api.waitForResponseReady(id);
	Request* r = api.getRequestByID(id);
	if (r->httpStatusCode() == 204)
		return nullptr;
	else if (r->httpStatusCode() == 202) {
		//TODO check status
		return nullptr;
	}
	else throw new UnexpectedHttpStatusCode(r->httpStatusCode());
}

void YandexDiskREST::mkdir(const QString& dir)
{
	QNetworkRequest req(apiURL + "resources/?path=" + dir);
	req.setRawHeader("Authorization", authorizationHeader);
	RequestID id = api.GET(req);
	api.waitForResponseReady(id);
	Request* r = api.getRequestByID(id);
	if (r->httpStatusCode() != 201)
		throw new UnexpectedHttpStatusCode(r->httpStatusCode());
}



YandexDiskREST::YandexDiskREST(QString qsettingsGroup /*= QString()*/)
	: AbstractCloud(qsettingsGroup), api(apiURL), disk(apiURL, false)
{
	if (!qsettingsGroup.isEmpty()) {
		QSettings settings;
		settings.beginGroup(qsettingsGroup);
		authorizationHeader = settings.value("authorization").value<QByteArray>();	//TODO encrypt token
		settings.endGroup();
	}
}

YandexDiskREST::~YandexDiskREST()
{
	if (!qsettingsGroup.isEmpty()) {
		QSettings settings;
		settings.beginGroup(qsettingsGroup);
		settings.setValue("authorization", QVariant::fromValue(authorizationHeader));
		settings.endGroup();
	}
}



QDateTime YandexDiskREST::lastModified(const ShortName& name) const
{
	QNetworkRequest req(apiURL + "resources/?fields=modified&path=" + name);
	req.setRawHeader("Authorization", authorizationHeader);
	RequestID id = api.GET(req);
	api.waitForResponseReady(id);
	Request* r = api.getRequestByID(id);
	if (r->httpStatusCode() != 201)
		throw new UnexpectedHttpStatusCode(r->httpStatusCode());
	QJsonObject fileInfo = QJsonDocument::fromJson(r->responseBody()).object();
	QString datetime = fileInfo["modified"].toString();
	return QDateTime::fromString(datetime, Qt::ISODate);
}

QByteArray YandexDiskREST::remoteMD5FileHash(const ShortName& filename) const
{
	QNetworkRequest req(apiURL + "resources/?fields=md5&path=" + filename);
	req.setRawHeader("Authorization", authorizationHeader);
	RequestID id = api.GET(req);
	api.waitForResponseReady(id);
	Request* r = api.getRequestByID(id);
	if (r->httpStatusCode() != 201)
		throw new UnexpectedHttpStatusCode(r->httpStatusCode());
	QJsonObject fileInfo = QJsonDocument::fromJson(r->responseBody()).object();
	return fileInfo["md5"].toString().toLocal8Bit();
}



QString YandexDiskREST::userName() const
{
	QUrl userInfoUrl("https://login.yandex.ru/info");
	RequestManager login(userInfoUrl);
	QNetworkRequest req(userInfoUrl);
	req.setRawHeader("Authorization", authorizationHeader);
	RequestID id = api.GET(req);
	api.waitForResponseReady(id);
	Request* r = api.getRequestByID(id);
	if (r->httpStatusCode() != 200)
		throw new UnexpectedHttpStatusCode(r->httpStatusCode());
	QJsonObject userInfo = QJsonDocument::fromJson(r->responseBody()).object();
	return userInfo["login"].toString();
}



QByteArray YandexDiskREST::sendDebugRequest(QByteArray requestType, QString url, QByteArray body /*= QByteArray()*/, QList<QNetworkReply::RawHeaderPair> additionalHeaders /*= QList<QNetworkReply::RawHeaderPair>()*/)
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
}












