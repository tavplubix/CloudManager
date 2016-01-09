#include "RequestManager.h"

#include <QBuffer>
#include <QJsonDocument>
#include <QEventLoop>











Request RequestManager::newRequest(QNetworkReply* reply, QIODevice* body /*= nullptr*/)
{
	auto r = new RequestPrivate;
	if (body) r->setBody(body);
	r->setReply(reply);
	r->m_id = ++lastID;
	requestMap[lastID] = r;
	return r;
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



RequestPrivate * RequestManager::getRequestByID(RequestID id) const
{
	if (requestMap.contains(id))
		return requestMap[id];
	else
		return nullptr;
}

void RequestManager::waitForResponseReady(const Request& request) const
{
	if (request->status() == RequestPrivate::Status::ResponseReady)
		return;
	if (request->status() == RequestPrivate::Status::Finished)
		return;		//TODO throw
	QEventLoop loop;
	connect(request, &RequestPrivate::responseReady, &loop, &QEventLoop::quit);
	loop.exec();
}

Request RequestManager::HEAD(const QNetworkRequest & req)
{
	auto reply = head(req);
	return newRequest(reply);
}
Request RequestManager::GET(const QNetworkRequest & req)
{
	auto reply = get(req);
	return newRequest(reply);
}
Request RequestManager::DELETE(const QNetworkRequest & req)
{
	auto reply = deleteResource(req);
	return newRequest(reply);
}



Request RequestManager::POST(const QNetworkRequest & req, const QByteArray & body)
{
	auto buf = new QBuffer;
	buf->setData(body);
	buf->open(QIODevice::ReadOnly);
	auto reply = post(req, buf);
	return newRequest(reply, buf);
}
Request RequestManager::POST(const QNetworkRequest & req, const QJsonObject & body)
{
	auto qba_body = QJsonDocument(body).toJson();
	return POST(req, qba_body);
}
Request RequestManager::POST(const QNetworkRequest & req, QIODevice * body)
{
	body->open(QIODevice::ReadOnly);
	auto reply = post(req, body);
	return newRequest(reply, body);
}
//Request RequestManager::POST(const QNetworkRequest & req, const File & body)
//{
//	auto f = new QFile(body.localPath());
//	auto reply = post(req, f);
//	return newRequest(reply, f);
//}


Request RequestManager::PUT(const QNetworkRequest & req, const QByteArray & body)
{
	auto buf = new QBuffer;
	buf->setData(body);
	buf->open(QIODevice::ReadOnly);
	auto reply = put(req, buf);
	return newRequest(reply, buf);
}
Request RequestManager::PUT(const QNetworkRequest & req, const QJsonObject & body)
{
	auto qba_body = QJsonDocument(body).toJson();
	return PUT(req, qba_body);
}
Request RequestManager::PUT(const QNetworkRequest & req, QIODevice * body)
{
	body->open(QIODevice::ReadOnly);
	auto reply = put(req, body);
	return newRequest(reply, body);
}
//Request RequestManager::PUT(const QNetworkRequest & req, const File & body)
//{
//	auto f = new QFile(body.localPath());
//	auto reply = put(req, f);
//	return newRequest(reply, f);
//}


[[deprecated("You better DO NOT use this method")]]
const QNetworkReply * RequestManager::__CRUTCH__get_QNetworkReply_object_by_Request__(Request& r) const
{
	return r->m_reply;
}
[[deprecated("warning: converting object of deprecated ReplyID to object of new class Request")]]
Request RequestManager::fromDeprecatedID(const RequestManager::ReplyID rid)
{
	Request r = new RequestPrivate;
	r->setReply(rid);
	r->m_id = 0;
	connect(rid, &QNetworkReply::destroyed, r, &RequestPrivate::deleteLater);	//WARNING
	return r;
}













RequestPrivate::RequestPrivate(QObject* parent /*= nullptr*/)
	: QObject(parent)
{
	m_id = 0;
	m_reply = nullptr;
	m_body = nullptr;
	m_status = Status::Empty;
}

RequestPrivate::~RequestPrivate()
{
	if (m_reply && m_id)
		delete m_reply;
	if (m_body) {
		m_body->close();
		delete m_body;
	}
}

void RequestPrivate::setReply(QNetworkReply* reply)
{
	if (m_reply)
		delete m_reply;		//WARNING
	m_reply = reply;
	m_status = Status::InProcess;
	connect(m_reply, &QNetworkReply::finished, this, &RequestPrivate::finishedSlot);
}

void RequestPrivate::setBody(QIODevice * body)
{	
	//m_body must remain valid until m_reply is finished
	if (m_reply) 
		if (m_reply->isRunning())
			m_reply->abort();
	
	if (m_body)
		delete m_body;
	m_body = body;
}

void RequestPrivate::waitForResponseReady() const
{
	if (m_status == Status::ResponseReady)
		return;
	if (m_status == Status::Finished)
		return;
	QEventLoop loop;
	connect(this, &RequestPrivate::responseReady, &loop, &QEventLoop::quit);
	loop.exec();
}

HeaderList RequestPrivate::responseHeaders() const
{
	if (m_status != Status::ResponseReady)
		return HeaderList();	//TODO throw
	else
		return m_reply->rawHeaderPairs();
}

QByteArray RequestPrivate::responseBody() const
{
	if (m_status != Status::ResponseReady)
		return QByteArray();	//TODO throw
	else
		return m_reply->readAll();
}

int RequestPrivate::httpStatusCode()
{
	return m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();	//WARNING
}

void RequestPrivate::finishedSlot()
{
	m_status = Status::ResponseReady;
	emit responseReady();
}


