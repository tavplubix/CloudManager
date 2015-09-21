#include "CustomRequestDialog.h"
#include "AbstractCloud.h"
#include "FileClasses.h"
#include <QUrl>

void CustomRequestDialog::clear()
{
	headers.clear();
	ui.headers->clear();
	ui.body->clear();
	ui.log->clear();
}

void CustomRequestDialog::send()
{
	QByteArray method = ui.method->currentText().toLocal8Bit();
	QString url = ui.url->text();
	QString filename = QUrl(url).path();
	filename.remove(0, 1);
	if (filename.isEmpty()) filename = ".";
	QByteArray body = ui.body->toPlainText().toLocal8Bit();
	if (method == "lastModified") {
		QDateTime qdt = cloud->lastModified(filename);
		ui.log->appendPlainText("\nTime: " + qdt.toString() + "\n");
	}
	else if (method == "remoteMD5FileHash") {
		QByteArray md5 = cloud->remoteMD5FileHash(filename);
		ui.log->appendPlainText("\nHash: " + md5 + "\n");
	}
	else if (method == "downloadFile") {
		auto buf = QSharedPointer<QBuffer>(new QBuffer);
		cloud->waitFor(cloud->downloadFile(filename, buf));
		ui.log->appendPlainText("\nDownloaded File: \n" + buf->data() + "\n");
	}
	else if (method == "uploadFile") {
		auto buf = new QBuffer;
		buf->setData(body);
		cloud->waitFor(cloud->uploadFile(filename, buf));
		ui.log->appendPlainText("\nFile Uploaded\n");
	}
	else if (method == "remove") {
		cloud->waitFor(cloud->remove(filename));
		ui.log->appendPlainText("\nFile Removed\n");
	}
	else if (method == "spaceAvailable") {
		ui.log->appendPlainText("\nBytes Available: " + QString::number(cloud->spaceAvailable()) + "\n");
	}
	else {
		QByteArray log = cloud->sendDebugRequest(method, url, body, headers);
		ui.log->appendPlainText(log);
	}
}

void CustomRequestDialog::addHeader()
{
	QNetworkReply::RawHeaderPair header;
	header.first = ui.newHeaderName->text().toLocal8Bit();
	header.second = ui.newHeaderValue->text().toLocal8Bit();
	ui.headers->appendPlainText(header.first + ": " + header.second + "\n");
	headers.push_back(header);
}

CustomRequestDialog::CustomRequestDialog(AbstractCloud *cloud, QWidget* parent)
	: QDialog(parent), cloud(cloud)
{
	qInfo("CustomRequestDialog()");
	ui.setupUi(this);
	setWindowTitle(cloud->userName() + "@" + cloud->serviceName());
	setLayout(ui.gridLayout);
	//ui.centralWidget->setLayout(ui.gridLayout);
	connect(ui.clear, &QPushButton::clicked, this, &CustomRequestDialog::clear);
	connect(ui.addHeader, &QPushButton::clicked, this, &CustomRequestDialog::addHeader);
	connect(ui.send, &QPushButton::clicked, this, &CustomRequestDialog::send);
}


CustomRequestDialog::~CustomRequestDialog()
{
	qInfo("~CustomRequestDialog()");
}
