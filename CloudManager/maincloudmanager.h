#pragma once


#include <QtWidgets/QMainWindow>
#include "ui_maincloudmanager.h"
//#include "CommonIncludes.h"
#include "CloudManager.h"
#include <QFileDialog>

class MainCloudManager : public QMainWindow
{
	Q_OBJECT

public:
	MainCloudManager(QWidget *parent = 0);
	~MainCloudManager();

private:
	CloudManager* manager;
	//QList<AbstractCloud*> stroages;
	Ui::MainCloudManagerClass ui;
	const QDir rootDir = "F:/Backups/CloudManager/";	//Hardcoded temporary
	void restoreGeometry();
	void saveGeometry();
private slots:
	void refresh();
	void remove();
	void sync();
	void customRequest();
	void addCloud();
	void removeCloud();
};


