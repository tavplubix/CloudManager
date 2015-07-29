#include "QAbstractManager.h"


void QAbstractManager::netLog(QNetworkReply *reply)
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
}

void QAbstractManager::netLog(const QNetworkRequest &request, const QByteArray &body)
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
}


QAbstractManager::Status QAbstractManager::currentStatus()
{
	return status;
}

void QAbstractManager::setActionWithChanged(ActionWithChanged act)
{
	action = act;
}

void QAbstractManager::syncAll()
{
	/*downloadAllNew();		//WARNING
	uploadAllNew();			//WARNING
	switch (action) {
	case ActionWithChanged::Pull :
		pullChangedFiles();
		break;
	case ActionWithChanged::Push :
		pushChangedFiles();
		break;
	}*/
}

QAbstractManager::~QAbstractManager()
{
	delete settings;
}
