#include "CustomRequestDialog.h"
#include "QAbstractManager.h"

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
	QByteArray body = ui.body->toPlainText().toLocal8Bit();
	QByteArray log = manager->sendDebugRequest(method, ui.url->text(), body, headers);
	ui.log->setPlainText(log);
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
