#include "YandexDiskManager.h"


YandexDiskManager::YandexDiskManager()
{
	oauth = nullptr;
	status = Status::Init;
	//restore settings
	settings->beginGroup("yadisk");
		authtype = static_cast<AuthType>( settings->value("aythtype", static_cast<int>(AuthType::OAuth)).toInt() );		// I really love static typing
		authorizationHeader = settings->value("authorization").value<QByteArray>();	//TODO шифровать токен
	settings->endGroup();
	//есть смысл делать всё асинхронно
	disk.connectToHostEncrypted("https://webdav.yandex.ru/");
	//QSettings
	if (!authorized()) QTimer::singleShot(0, this, &YandexDiskManager::authorize);
}

YandexDiskManager::~YandexDiskManager()
{
	//save settings
	settings->beginGroup("yadisk");
		settings->setValue("authtype", static_cast<int>(authtype));
		settings->setValue("authorization", QVariant::fromValue(authorizationHeader));
	settings->endGroup();
	//TODO завершить все операции
}


void YandexDiskManager::authorize()
{
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
			status = Status::Failed;
			return;
		}

		//Запрос токена
		if (oauth == nullptr) oauth = new QNetworkAccessManager(this);
		QUrl authServer = authURL;
		oauth->connectToHostEncrypted("https://oauth.yandex.ru");	//WARNING 
		QByteArray requestBody = "grant_type=authorization_code";
		requestBody += "&code=" + QString::number(verificationCode);
		requestBody += "&client_id=" + appID;
		requestBody += "&client_secret=" + clientSecret;
		//requestBody += "&device_id=" + deviceID;
		//requestBody += "&device_name=" + deviceName;
		QNetworkReply *reply = oauth->post(QNetworkRequest(QUrl("https://oauth.yandex.ru/token")), QByteArray(requestBody));
		netLog(QNetworkRequest(QUrl("https://oauth.yandex.ru/token")), requestBody);	//DEBUG
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
				status = Status::Ready;
			}
			else {
				QTimer::singleShot(1000, this, &YandexDiskManager::authorize);
			}
		} );
	}
}



qint64 YandexDiskManager::spaceAvailable()		//FIXME spaceAvailable()
//варианты:
// 1 заблокировать поток, отправить запрос в другом потоке (Блокировка потока - плохо)
// 2 добавить функцию updateAvailableSpace(), из этой ф-ции возвращать неактуальные данные
// 3 возвращать значение сигналом (хуйня?)
// 4 осилить XMPP (второй вариант + получать уведомления об изменении доступного объёма, инфа актуальна)  -  xmpp _СЛИШКОМ_ ебаный, лучше ВРЕМЕННО закостылять первым способом
{
	
	QNetworkRequest request(QUrl("https://webdav.yandex.ru/"));
	request.setRawHeader("AUthorization", authorizationHeader);
	request.setRawHeader("Depth", "0");
	QByteArray body = "<propfind xmlns=\"DAV:\"> <prop> <quota-available-bytes/> <quota-used-bytes/> </prop> </propfind>";
	QBuffer *buf = new QBuffer;
	buf->setData(body);
	QNetworkReply *reply = disk.sendCustomRequest(request, "PROPFIND", buf);
	netLog(request, body);
	buf->setParent(reply);

	QEventLoop loop;
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();


	checkForHTTPErrors(reply);
	QByteArray response = reply->readAll();
	response.toLower();
	int begin = response.indexOf("<d:quota-available-bytes>");
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

void YandexDiskManager::setAuthType(AuthType type)
{
	//WARNING This function may cause an error if any operations are performed.
	authorizationHeader.clear();
	authtype = type;
	authorize();
}

bool YandexDiskManager::_checkForHTTPErrors(QNetworkReply *reply, const char* file, const int line, const char* func)
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

bool YandexDiskManager::authorized()
{
	return !authorizationHeader.isEmpty();		//TODO check token lifetime
}



void YandexDiskManager::downloadFile(QFileInfo file)
{
	auto dbg = file.absoluteFilePath();
	dbg = file.canonicalFilePath();
	dbg = file.filePath();
	if (file.isRelative()) file = QFileInfo(rootDir.absoluteFilePath(file.filePath()));
	dbg = file.canonicalFilePath();
	dbg = file.absoluteFilePath();
	QString relative = rootDir.relativeFilePath(file.absoluteFilePath());
	QNetworkRequest request("https://webdav.yandex.ru/" + relative);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.get(request);
	netLog(request, "EMPTY");	//DEBUG
	//connect(reply, &QNetworkReply::finished, this, &YandexDiskManager::fileDownloaded);		//хуйня, лямды лучше
	connect(reply, &QNetworkReply::finished, this, [=](){ 
		bool ok = false;
		checkForHTTPErrors(reply);
		QByteArray MD5hash = reply->rawHeader("Etag");
		QFile downloaded(file.absoluteFilePath());
		downloaded.open(QIODevice::WriteOnly);
		downloaded.write(reply->readAll());
		downloaded.close();
		//check md5
		QCryptographicHash hashCalculator(QCryptographicHash::Algorithm::Md5);
		hashCalculator.addData(&downloaded);
		QByteArray fileHash = hashCalculator.result().toHex();
		if (fileHash == MD5hash) ok = true;
		if (!ok) qDebug() << QString::fromLocal8Bit("Хуйня");
	});		

}


void YandexDiskManager::uploadFile(QFileInfo file)	//FIXME segfault
{
	//WARNING может не работать для файлов не в корне
	QString relative;
	if (file.isAbsolute()) relative = rootDir.relativeFilePath(file.absoluteFilePath());
	else relative = file.filePath();
	QNetworkRequest request("https://webdav.yandex.ru/" + relative);
	request.setRawHeader("Authorization", authorizationHeader);
	QFile *f = new QFile(file.absoluteFilePath());
	QCryptographicHash md5HashCalculator(QCryptographicHash::Algorithm::Md5);
	md5HashCalculator.addData(f);
	request.setRawHeader("Etag", md5HashCalculator.result().toHex());
	QCryptographicHash sha256HashCalculator(QCryptographicHash::Algorithm::Sha256);
	sha256HashCalculator.addData(f);
	request.setRawHeader("Sha256", sha256HashCalculator.result().toHex());
	QNetworkReply *reply = disk.put(request, f);
		//f.open(QIODevice::ReadOnly);
		//QByteArray tmp = f.readAll();
		//QNetworkReply *reply = disk.put(request, tmp);
	f->setParent(reply);
	netLog(request, "FILE");	//DEBUG
	connect(reply, &QNetworkReply::finished, this, [=](){
		checkForHTTPErrors(reply);
		int statusCode = HTTPstatus(reply);
		if (statusCode == 100) return;
		if (statusCode == 201) {
			reply->abort();
			reply->deleteLater();
			return;
		}
	});
}

void YandexDiskManager::mkdir(QDir dir)
{
	QString relative;
	if(dir.isAbsolute()) relative = rootDir.relativeFilePath(dir.absolutePath());		//WARNING не работает для дирректорий не в корне
	else relative = dir.path();
	QNetworkRequest request("https://webdav.yandex.ru/" + relative);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.sendCustomRequest(request, "MKCOL");
	netLog(request, "EMPTY");	//DEBUG
	connect(reply, &QNetworkReply::finished, this, [=](){
		checkForHTTPErrors(reply);
		reply->deleteLater();
	});
}

void YandexDiskManager::remove(QFileInfo file)
{
	QString relative;
	if (file.isAbsolute()) relative = rootDir.relativeFilePath(file.absoluteFilePath());
	else relative = file.filePath();
	QNetworkRequest request("https://webdav.yandex.ru/" + relative);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.sendCustomRequest(request, "DELETE");
	netLog(request, "EMPTY");	//DEBUG
	connect(reply, &QNetworkReply::finished, this, [=](){
		checkForHTTPErrors(reply);
		reply->deleteLater();
	});
}




