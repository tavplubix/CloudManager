#include "YandexDiskManager.h"


YandexDiskManager::YandexDiskManager()
{
	oauth = nullptr;
	status = Status::Init;
	authtype = AuthType::OAuth;
	//есть смысл делать всё асинхронно
	disk.connectToHostEncrypted("https://webdav.yandex.ru/");
	//QSettings
	QTimer::singleShot(0, this, &YandexDiskManager::authorize);
}

YandexDiskManager::~YandexDiskManager()
{
	//TODO завершить все операции
}


//=============================================================================
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
		QString label = QString::fromLocal8Bit("Введите код верификации. Его можно получить по указанной ссылке.\n");
		label += authURL;
		int verificationCode = QInputDialog::getInt(qApp->activeWindow(), QString::fromLocal8Bit("Авторизация"), label, 2336312, 0);
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
//void YandexDiskManager::tokenGot()
//{
//	bool ok = false;
//	QNetworkReply *reply = static_cast<QNetworkReply*> (sender());
//	int HTTPstatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
//	if (HTTPstatus != 200)
//		qDebug() << "Token request: HTTP status: " << QString(HTTPstatus);
//	QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
//	reply->deleteLater();
//	if (response.contains("token")) {
//		authorizationHeader = "OAuth " + response["token"].toString().toLocal8Bit();
//		validUntil = time(nullptr) + response["exprires_in"].toInt();
//		ok = true;
//	}
//	else {
//		qDebug() << "Token request: Error: " << response["error_description"].toString() << " Code: " << response["error"].toString();
//	}
//	if (ok) {
//		delete oauth;
//		oauth = nullptr;
//		status = Status::Ready;
//	}
//	else {
//		QTimer::singleShot(1000, &YandexDiskManager::authorize);
//	}
//}
//=============================================================================



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
	return !authorizationHeader.isEmpty();
}




//=============================================================================
void YandexDiskManager::downloadFile(QFileInfo file)
{
	if (file.isRelative()) file = QFileInfo(rootDir.absoluteFilePath(file.canonicalFilePath()));
	if (!downloadingFile.isEmpty()) {
		qDebug() << "YandexDiskManager::downloadFile(): WARNING: file ignored\n";
		return;
	}
	QString relative = rootDir.relativeFilePath(file.canonicalFilePath());
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
		QByteArray fileHash = hashCalculator.result();
		if (fileHash == MD5hash) ok = true;
		if (!ok) qDebug() << "Хуйня";
	});		

}
//void YandexDiskManager::fileDownloaded()	//не нужна
//{
//	bool ok = false;
//	QNetworkReply *reply = static_cast<QNetworkReply*> (sender());
//	QVariant vMD5 = reply->rawHeader("Etag");
//	//QString f = reply->request().url().toString();
//	QFile file();
//}
//=============================================================================



void YandexDiskManager::uploadFile(QFileInfo file)
{
	QString relative = rootDir.relativeFilePath(file.canonicalFilePath());		//WARNING может не работать для файлов не в корне
	QNetworkRequest request("https://webdav.yandex.ru/" + relative);
	request.setRawHeader("Authorization", authorizationHeader);
	QFile f(file.absoluteFilePath());
	QCryptographicHash md5HashCalculator(QCryptographicHash::Algorithm::Md5);
	md5HashCalculator.addData(&f);
	request.setRawHeader("Etag", md5HashCalculator.result());
	QCryptographicHash sha256HashCalculator(QCryptographicHash::Algorithm::Sha256);
	sha256HashCalculator.addData(&f);
	request.setRawHeader("Sha256", sha256HashCalculator.result());
	QNetworkReply *reply = disk.put(request, &f);
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
	QString relative = rootDir.relativeFilePath(dir.canonicalPath());		//WARNING не работает для дирректорий не в корне
	QNetworkRequest request(relative);
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
	QString relative = rootDir.relativeFilePath(file.canonicalFilePath());		
	QNetworkRequest request(relative);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.sendCustomRequest(request, "DELETE");
	netLog(request, "EMPTY");	//DEBUG
	connect(reply, &QNetworkReply::finished, this, [=](){
		checkForHTTPErrors(reply);
		reply->deleteLater();
	});
}




