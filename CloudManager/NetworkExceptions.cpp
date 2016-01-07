#include <NetworkExceptions.h>

UnexpectedHttpStatusCode::UnexpectedHttpStatusCode(int code)
{
	m_statusCode = code;
}

int UnexpectedHttpStatusCode::statusCode()
{
	return m_statusCode;
}

HttpError::HttpError(int code)
	: UnexpectedHttpStatusCode(code)
{
	
}

bool HttpError::isClientError()
{
	return m_statusCode / 100 == 4;
}

bool HttpError::isServerError()
{
	return m_statusCode / 100 == 5;
}


int checkForHTTPErrors_maythrow(QNetworkReply* reply, QList<int> expected /*= QList<int>({ 200 })*/) {
	int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (expected.contains(status))
		return status;
	if (status == 404)
		throw new Http404NotFound;
	else if (status == 403)
		throw new Http403Forbidden;
	else if (status == 401)
		throw new Http401Unauthorized;
	else if (status >= 400)
		throw new HttpError(status);
	else throw new UnexpectedHttpStatusCode(status);
}

