#pragma once
#include <QDialog>
#include <QList>
#include <QNetworkReply>
#include "ui_customRequestDialog.h"

class AbstractCloud;

class CustomRequestDialog :
	public QDialog
{
	Q_OBJECT
private:
	Ui::CustomRequestDialogUI ui;
	QList<QNetworkReply::RawHeaderPair> headers;
	AbstractCloud* cloud;
private slots:
	void clear();
	void send();
	void addHeader();
public:
	CustomRequestDialog(AbstractCloud* cloud, QWidget* parent = nullptr);
	~CustomRequestDialog();
};

