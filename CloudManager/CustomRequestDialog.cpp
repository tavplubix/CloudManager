#include "CustomRequestDialog.h"
#include "QAbstractManager.h"
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
		QDateTime qdt = manager->lastModified(filename);
		ui.log->appendPlainText("\nTime: " + qdt.toString() + "\n");
	}
	else if (method == "remoteMD5FileHash") {
		QByteArray md5 = manager->remoteMD5FileHash(filename);
		ui.log->appendPlainText("\nHash: " + md5 + "\n");
	}
	else if (method == "downloadFile") {
		auto buf = QSharedPointer<QBuffer>(new QBuffer);
		manager->waitFor(manager->downloadFile(filename, buf));
		ui.log->appendPlainText("\nDownloaded File: \n" + buf->data() + "\n");
	}
	else if (method == "uploadFile") {
		auto buf = new QBuffer;
		buf->setData(body);
		manager->waitFor(manager->uploadFile(filename, buf));
		ui.log->appendPlainText("\nFile Uploaded\n");
	}
	else if (method == "remove") {
		manager->waitFor(manager->remove(filename));
		ui.log->appendPlainText("\nFile Removed\n");
	}
	else if (method == "spaceAvailable") {
		ui.log->appendPlainText("\nBytes Available: " + QString::number(manager->spaceAvailable()) + "\n");
	}
	else {
		QByteArray log = manager->sendDebugRequest(method, url, body, headers);
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

CustomRequestDialog::CustomRequestDialog(QAbstractManager *manager, QWidget* parent)
	: QDialog(parent), manager(manager)
{
	ui.setupUi(this);
	setLayout(ui.gridLayout);
	//ui.centralWidget->setLayout(ui.gridLayout);
	connect(ui.clear, &QPushButton::clicked, this, &CustomRequestDialog::clear);
	connect(ui.addHeader, &QPushButton::clicked, this, &CustomRequestDialog::addHeader);
	connect(ui.send, &QPushButton::clicked, this, &CustomRequestDialog::send);
}


CustomRequestDialog::~CustomRequestDialog()
{
}
