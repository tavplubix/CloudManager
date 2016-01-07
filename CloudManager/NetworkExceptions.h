#pragma once
#include <QException>
#include <QNetworkReply>
#include <QList>

typedef QList<QException*> ExceptionList;

class UnexpectedHttpStatusCode : public QException {
protected:
	int m_statusCode;
public:
	UnexpectedHttpStatusCode(int code);
	int statusCode();
};

class HttpError : public UnexpectedHttpStatusCode {
public:
	HttpError(int code);
	bool isClientError();
	bool isServerError();
	//TODO QString errorDescription();
};

class Http404NotFound : public HttpError {
public:
	Http404NotFound() : HttpError(404) {};
};

class Http403Forbidden : public HttpError {
public:
	Http403Forbidden() : HttpError(403) {};
};

class Http401Unauthorized : public HttpError {
public:
	Http401Unauthorized() : HttpError(401) {};
};

class NetworkUnavailable : public QException {};


extern int checkForHTTPErrors_maythrow(QNetworkReply* reply, QList<int> expected = QList<int>({200}));





