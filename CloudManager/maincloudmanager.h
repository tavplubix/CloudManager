#ifndef MAINCLOUDMANAGER_H
#define MAINCLOUDMANAGER_H

#include <QtWidgets/QMainWindow>
#include "ui_maincloudmanager.h"
//#include "CommonIncludes.h"
#include "YandexDiskManager.h"
#include <QFileDialog>

class MainCloudManager : public QMainWindow
{
	Q_OBJECT

public:
	MainCloudManager(QWidget *parent = 0);
	~MainCloudManager();

private:
	QList<QAbstractManager*> stroages;
	Ui::MainCloudManagerClass ui;
	const QDir rootDir = "F:/Backups/CloudManager/";	//Hardcoded temporary
private slots:
	void refresh();
	void remove();
	void sync();
};

#endif // MAINCLOUDMANAGER_H
