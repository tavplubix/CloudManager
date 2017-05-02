#include "YandexDiskManager.h"


YandexDiskManager::YandexDiskManager()
{
	oauth = nullptr;
	m_status = Status::Init;
	//restore settings
	settings->beginGroup("yadisk");
		authtype = static_cast<AuthType>( settings->value("aythtype", static_cast<int>(AuthType::OAuth)).toInt() );		// I really love static typing
		authorizationHeader = settings->value("authorization").value<QByteArray>();	//TODO ��������� �����
	settings->endGroup();
	//���� ����� ������ �� ����������
	disk.connectToHostEncrypted("https://webdav.yandex.ru/");
	//QSettings
	//if (!authorized()) QTimer::singleShot(0, this, &YandexDiskManager::authorize);
}

YandexDiskManager::~YandexDiskManager()
{
	//save settings
	settings->beginGroup("yadisk");
		settings->setValue("authtype", static_cast<int>(authtype));
		settings->setValue("authorization", QVariant::fromValue(authorizationHeader));
	settings->endGroup();
	//TODO ��������� ��� ��������
}


QNetworkReply* YandexDiskManager::authorize()
{
	if (authtype == AuthType::Basic) {
		qDebug() << "Basic authorization isn't implemented\n";
		authtype = AuthType::OAuth;
	}
	if (authtype == AuthType::OAuth) {
		//��������� ���� �����������
		QString authURL = "https://oauth.yandex.ru/authorize?response_type=code";
		authURL += "&client_id=" + appID;
		//authURL += "&device_id=" + deviceID;
		//authURL += "&device_name=" + deviceName;
		QString label = QString::fromLocal8Bit("������� ��� �����������. ��� ����� �������� <a href="); 
		label += "\"" + authURL + "\"" + QString::fromLocal8Bit(">�����</a>.\n");
		int verificationCode = QInputDialog::getInt(qApp->activeWindow(), QString::fromLocal8Bit("�����������"), label, -1, 0);
		if (verificationCode < 0) {
			m_status = Status::Failed;
			return;
		}

		//������ ������
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
				emit done();
			}
			else {
				QTimer::singleShot(1000, this, &YandexDiskManager::authorize);
			}
		} );
		return reply;
	}
}



qint64 YandexDiskManager::spaceAvailable()	const	//FIXME spaceAvailable()
//��������:
// 1 ������������� �����, ��������� ������ � ������ ������ (���������� ������ - �����)
// 2 �������� ������� updateAvailableSpace(), �� ���� �-��� ���������� ������������ ������
// 3 ���������� �������� �������� (�����?)
// 4 ������� XMPP (������ ������� + �������� ����������� �� ��������� ���������� ������, ���� ���������)  -  xmpp _�������_ ������, ����� �������� ����������� ������ ��������
{
	
	QNetworkRequest request(QUrl("https://webdav.yandex.ru/"));
	request.setRawHeader("AUthorization", authorizationHeader);
	request.setRawHeader("Depth", "0");
	QByteArray body = "<propfind xmlns=\"DAV:\"> <prop> <quota-available-bytes/> <quota-used-bytes/> </prop> </propfind>";
	QBuffer *buf = new QBuffer;
	buf->setData(body);
	QNetworkReply *reply = disk.sendCustomRequest(request, "PROPFIND", buf);
	netLog("PROPFIND", request, body);
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

bool YandexDiskManager::_checkForHTTPErrors(QNetworkReply *reply, const char* file, const int line, const char* func) const
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

bool YandexDiskManager::authorized() const
{
	return !authorizationHeader.isEmpty();		//TODO check token lifetime
}



QNetworkReply* YandexDiskManager::downloadFile(const QString& name, QIODevice* file)
{
	
	//if (name.isRelative()) name = QFileInfo(rootDir.absoluteFilePath(name.filePath()));
	//QString relative = rootDir.relativeFilePath(name.absoluteFilePath());
	QNetworkRequest request("https://webdav.yandex.ru/" + name/*relative*/);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.get(request);
	netLog("GET", request, "EMPTY");	//DEBUG
	//connect(reply, &QNetworkReply::finished, this, &YandexDiskManager::fileDownloaded);		//�����, ����� �����
	connect(reply, &QNetworkReply::finished, this, [=](){ 
		bool ok = false;
		checkForHTTPErrors(reply);
		QByteArray MD5hash = reply->rawHeader("Etag");
		//QFile downloaded(name/*.absoluteFilePath()*/);
		file->open(QIODevice::WriteOnly);
		file->write(reply->readAll());
		file->close();
		//check md5
		QCryptographicHash hashCalculator(QCryptographicHash::Algorithm::Md5);
		file->open(QIODevice::ReadOnly);		//WARNING � ���� ��� �� ����?
		hashCalculator.addData(file);
		file->close();
		QByteArray fileHash = hashCalculator.result().toHex();
		if (fileHash == MD5hash) ok = true;
		if (!ok) qDebug() << QString::fromLocal8Bit("�����");
		delete file;
		emit done();
	});	
	return reply;
}


QNetworkReply* YandexDiskManager::uploadFile(const QString& name, QIODevice* file)	
{
	//WARNING ����� �� �������� ��� ������ �� � �����
	//QString relative;
	//if (file.isAbsolute()) relative = rootDir.relativeFilePath(file.absoluteFilePath());
	//else relative = file.filePath();
	QNetworkRequest request("https://webdav.yandex.ru/" + name/*relative*/);
	request.setRawHeader("Authorization", authorizationHeader);
	//QFile *f = //new QFile(name/*file.absoluteFilePath()*/);
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
		//f.open(QIODevice::ReadOnly);
		//QByteArray tmp = f.readAll();
		//QNetworkReply *reply = disk.put(request, tmp);
	file->setParent(reply);		//WARNING
	netLog("PUT", request, "FILE");	//DEBUG
	connect(reply, &QNetworkReply::finished, this, [=](){
		checkForHTTPErrors(reply);
		int statusCode = HTTPstatus(reply);
		if (statusCode == 100) return;
		if (statusCode == 201) {
			reply->abort();
			reply->deleteLater();
			delete file;	//WARNING ������ ������ � ������ ������ (�� 201)
			emit done();
			return;
		}
	}); 
	return reply;
}

QDateTime YandexDiskManager::lastModified(QFileInfo file) const
{
	QString relative;
	if (file.isAbsolute()) relative = rootDir.relativeFilePath(file.absoluteFilePath());
	else relative = file.filePath();
	QNetworkRequest request("https://webdav.yandex.ru/" + relative);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.head(request);
	netLog("HEAD", request, "EMPTY");
	QEventLoop loop;
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();
	checkForHTTPErrors(reply);
	QByteArray time = reply->rawHeader("Last-Modified");
	if (time.isEmpty()) return QDateTime::fromTime_t(0);
	return QDateTime::fromString(time, Qt::RFC2822Date);
}

void YandexDiskManager::mkdir(QDir dir)
{
	QString relative;
	if(dir.isAbsolute()) relative = rootDir.relativeFilePath(dir.absolutePath());		//WARNING �� �������� ��� ����������� �� � �����
	else relative = dir.path();
	QNetworkRequest request("https://webdav.yandex.ru/" + relative);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.sendCustomRequest(request, "MKCOL");
	netLog("MKCOL", request, "EMPTY");	//DEBUG
	connect(reply, &QNetworkReply::finished, this, [=](){
		checkForHTTPErrors(reply);
		reply->deleteLater();
		emit done();
	});
}

QNetworkReply* YandexDiskManager::remove(const QString& name)
{
	//QString relative;
	//if (name.isAbsolute()) relative = rootDir.relativeFilePath(name.absoluteFilePath());
	//else relative = name.filePath();
	QNetworkRequest request("https://webdav.yandex.ru/" + name/*relative*/);
	request.setRawHeader("Authorization", authorizationHeader);
	QNetworkReply *reply = disk.sendCustomRequest(request, "DELETE");
	netLog("DELETE", request, "EMPTY");	//DEBUG
	connect(reply, &QNetworkReply::finished, this, [=](){
		checkForHTTPErrors(reply);
		reply->deleteLater();
		emit done();
	});
	return reply;
}




