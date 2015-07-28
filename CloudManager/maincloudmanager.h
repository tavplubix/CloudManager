#ifndef MAINCLOUDMANAGER_H
#define MAINCLOUDMANAGER_H

#include <QtWidgets/QMainWindow>
#include "ui_maincloudmanager.h"
#include "CommonIncludes.h"
#include "YandexDiskManager.h"
#include <QFileDialog>

class MainCloudManager : public QMainWindow
{
	Q_OBJECT

public:
	MainCloudManager(QWidget *parent = 0);
	~MainCloudManager();

private:
	YandexDiskManager disk;
	Ui::MainCloudManagerClass ui;
	private slots:
	void choose();
	void download();
	void upload();
};

#endif // MAINCLOUDMANAGER_H
