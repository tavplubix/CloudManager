#pragma once
#include <QDialog>
#include <QList>
#include <QNetworkReply>
#include "ui_customRequestDialog.h"

class QAbstractManager;

class CustomRequestDialog :
	public QDialog
{
	Q_OBJECT
private:
	Ui::CustomRequestDialogUI ui;
	QList<QNetworkReply::RawHeaderPair> headers;
	QAbstractManager* manager;
private slots:
	void clear();
	void send();
	void addHeader();
public:
	CustomRequestDialog(QAbstractManager* manager, QWidget* parent = nullptr);
	~CustomRequestDialog();
};

