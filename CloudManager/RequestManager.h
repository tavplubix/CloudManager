#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QIODevice>

#include <QMap>
#include <QList>
#include <QPair>
#include <QPointer>

#include "Common.h"
#include "NetworkExceptions.h"
#include "File.h"



typedef unsigned long long RequestID;
typedef QList< QPair<QByteArray, QByteArray> > HeaderList;

class RequestManager;

class RequestPrivate : public QObject
{
	Q_OBJECT
public classes:
	enum class Status {
		Empty, Created, InProcess, ResponseReady, Finished, ErrorOcured
	};
private fields:
	RequestID m_id;
	QNetworkReply* m_reply;
	QIODevice* m_body;
	Status m_status;
	ExceptionList errors;
private slots:
	void finishedSlot();

private methods:
	friend RequestManager;
	RequestPrivate(QObject* parent = nullptr);
	~RequestPrivate();
	void setReply(QNetworkReply* reply);
	void setBody(QIODevice* body);

public methods:
	RequestID id() const noexcept { return m_id; }
	Status status() const noexcept { return m_status; }
	void waitForResponseReady() const;
	//void waitForFinished() const noexcept;
	HeaderList responseHeaders() const;
	QByteArray responseBody() const;
	int httpStatusCode();

signals:
	void responseReady();
};

typedef QPointer<RequestPrivate> Request;



class RequestManager : private QNetworkAccessManager 
{
	Q_OBJECT

private fields:
	RequestID lastID;
	QMap<RequestID, RequestPrivate*> requestMap;
		
private methods:
	Request newRequest(QNetworkReply* reply, QIODevice* body = nullptr);
public methods:
	RequestManager(const QUrl& host, bool encryped = true, QObject* parent = nullptr);
	RequestManager(const RequestManager&) = delete;
	RequestManager(const RequestManager&&) = delete;
	RequestManager& operator = (const RequestManager&) = delete;
	RequestManager& operator = (const RequestManager&&) = delete;

	RequestPrivate* getRequestByID(RequestID id) const;

	void waitForResponseReady(const Request& request) const;
	void waitForFinished(RequestID id) const noexcept;

	//Requests without body
	Request HEAD(const QNetworkRequest& req);
	Request GET(const QNetworkRequest& req);
	Request DELETE(const QNetworkRequest& req);

	//Requests with body
	Request POST(const QNetworkRequest& req, const QByteArray& body);
	Request POST(const QNetworkRequest& req, const QJsonObject& body);
	Request POST(const QNetworkRequest& req, const File& body);
	Request POST(const QNetworkRequest& req, QIODevice* body);
		   
	Request PUT(const QNetworkRequest& req, const QByteArray& body);
	Request PUT(const QNetworkRequest& req, const QJsonObject& body);
	Request PUT(const QNetworkRequest& req, const File& body);
	Request PUT(const QNetworkRequest& req, QIODevice* body);


	const QNetworkReply* __CRUTCH__get_QNetworkReply_object_by_Request__(Request& r) const;
	typedef QNetworkReply* ReplyID;
	static Request fromDeprecatedID(const ReplyID rid);

};

typedef QPointer<RequestPrivate> Request;












