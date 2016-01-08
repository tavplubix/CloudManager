#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QIODevice>

#include <QMap>
#include <QList>
#include <QPair>

#include "Common.h"
#include "NetworkExceptions.h"
#include "File.h"


typedef unsigned long long RequestID;
typedef QList< QPair<QByteArray, QByteArray> > HeaderList;

class RequestManager;

class Request : public QObject
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
	Request(QObject* parent = nullptr);
	~Request();
	void setReply(QNetworkReply* reply);
	void setBody(QIODevice* body);

public methods:
	RequestID id() const noexcept { return m_id; }
	Status status() const noexcept { return m_status; }
	//void waitForResponseReady() const;
	//void waitForFinished() const noexcept;
	HeaderList responseHeaders() const;
	QByteArray responseBody() const;
	int httpStatusCode();

signals:
	void responseReady();
};


class RequestManager : private QNetworkAccessManager 
{
	Q_OBJECT

private fields:
	RequestID lastID;
	QMap<RequestID, Request*> requestMap;
		
private methods:
	RequestID newRequest(QNetworkReply* reply, QIODevice* body = nullptr);
public methods:
	RequestManager(const QUrl& host, bool encryped = true, QObject* parent = nullptr);
	RequestManager(const RequestManager&) = delete;
	RequestManager(const RequestManager&&) = delete;
	RequestManager& operator = (const RequestManager&) = delete;
	RequestManager& operator = (const RequestManager&&) = delete;

	Request* getRequestByID(RequestID id) const;

	void waitForResponseReady(RequestID id) const;
	void waitForFinished(RequestID id) const noexcept;

	//Requests without body
	RequestID HEAD(const QNetworkRequest& req);
	RequestID GET(const QNetworkRequest& req);
	RequestID DELETE(const QNetworkRequest& req);

	//Requests with body
	RequestID POST(const QNetworkRequest& req, const QByteArray& body);
	RequestID POST(const QNetworkRequest& req, const QJsonObject& body);
	RequestID POST(const QNetworkRequest& req, const File& body);
	RequestID POST(const QNetworkRequest& req, QIODevice* body);
		   
	RequestID PUT(const QNetworkRequest& req, const QByteArray& body);
	RequestID PUT(const QNetworkRequest& req, const QJsonObject& body);
	RequestID PUT(const QNetworkRequest& req, const File& body);
	RequestID PUT(const QNetworkRequest& req, QIODevice* body);


	QNetworkReply* __CRUTCH__get_QNetworkReply_object_by_RequestID__(RequestID id);

};






