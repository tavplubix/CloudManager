#include "RequestManager.h"

#include <QBuffer>
#include <QJsonDocument>
#include <QEventLoop>









RequestID RequestManager::newRequest(QNetworkReply* reply, QIODevice* body /*= nullptr*/)
{
	auto r = new Request;
	if (body) r->setBody(body);
	r->setReply(reply);
	r->m_id = ++lastID;
	requestMap[lastID] = r;
	return r->m_id;
}

RequestManager::RequestManager(const QUrl& host, bool encryped /*= true*/, QObject* parent /*= nullptr*/)
	: QNetworkAccessManager(parent)
{
	if (encryped)
		connectToHostEncrypted(host.host());
	else
		connectToHost(host.host());
	lastID = 0;
}



Request * RequestManager::getRequestByID(RequestID id) const
{
	if (requestMap.contains(id))
		return requestMap[id];
	else
		return nullptr;
}

void RequestManager::waitForResponseReady(RequestID id) const
{
	auto request = getRequestByID(id);
	if (request->status() == Request::Status::ResponseReady)
		return;
	if (request->status() == Request::Status::Finished)
		return;		//TODO throw
	QEventLoop loop;
	connect(request, &Request::responseReady, &loop, &QEventLoop::quit);
	loop.exec();
}

RequestID RequestManager::HEAD(const QNetworkRequest & req)
{
	auto reply = head(req);
	return newRequest(reply);
}
RequestID RequestManager::GET(const QNetworkRequest & req)
{
	auto reply = get(req);
	return newRequest(reply);
}
RequestID RequestManager::DELETE(const QNetworkRequest & req)
{
	auto reply = deleteResource(req);
	return newRequest(reply);
}



RequestID RequestManager::POST(const QNetworkRequest & req, const QByteArray & body)
{
	auto buf = new QBuffer;
	buf->setData(body);
	buf->open(QIODevice::ReadOnly);
	auto reply = post(req, buf);
	return newRequest(reply, buf);
}
RequestID RequestManager::POST(const QNetworkRequest & req, const QJsonObject & body)
{
	auto qba_body = QJsonDocument(body).toJson();
	return POST(req, qba_body);
}
//RequestID RequestManager::POST(const QNetworkRequest & req, const File & body)
//{
//	auto f = new QFile(body.localPath());
//	auto reply = post(req, f);
//	return newRequest(reply, f);
//}


RequestID RequestManager::PUT(const QNetworkRequest & req, const QByteArray & body)
{
	auto buf = new QBuffer;
	buf->setData(body);
	buf->open(QIODevice::ReadOnly);
	auto reply = put(req, buf);
	return newRequest(reply, buf);
}
RequestID RequestManager::PUT(const QNetworkRequest & req, const QJsonObject & body)
{
	auto qba_body = QJsonDocument(body).toJson();
	return PUT(req, qba_body);
}
//RequestID RequestManager::PUT(const QNetworkRequest & req, const File & body)
//{
//	auto f = new QFile(body.localPath());
//	auto reply = put(req, f);
//	return newRequest(reply, f);
//}











Request::Request(QObject* parent /*= nullptr*/)
	: QObject(parent)
{
	m_id = 0;
	m_reply = nullptr;
	m_body = nullptr;
	m_status = Status::Empty;
}

Request::~Request()
{
	if (m_reply)
		delete m_reply;
	if (m_body)
		delete m_body;
}

void Request::setReply(QNetworkReply* reply)
{
	if (m_reply)
		delete m_reply;		//WARNING
	m_reply = reply;
	m_status = Status::InProcess;
	connect(m_reply, &QNetworkReply::finished, this, &Request::finishedSlot);
}

void Request::setBody(QIODevice * body)
{	
	//m_body must remain valid until m_reply is finished
	if (m_reply) 
		if (m_reply->isRunning())
			m_reply->abort();
	
	if (m_body)
		delete m_body;
	m_body = body;
}

//void Request::waitForResponseReady() const
//{
//	if (m_status == Status::ResponseReady)
//		return;
//	if (m_status == Status::Finished)
//		return;
//	QEventLoop loop;
//
//}

HeaderList Request::responseHeaders() const
{
	if (m_status != Status::ResponseReady)
		return HeaderList();	//TODO throw
	else
		return m_reply->rawHeaderPairs();
}

QByteArray Request::responseBody() const
{
	if (m_status != Status::ResponseReady)
		return QByteArray();	//TODO throw
	else
		return m_reply->readAll();
}

int Request::httpStatusCode()
{
	return m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();	//WARNING
}

void Request::finishedSlot()
{
	m_status = Status::ResponseReady;
	emit responseReady();
}

