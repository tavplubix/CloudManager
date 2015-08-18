#include "maincloudmanager.h"
#include "FileClasses.h"
#include "CommonIncludes.h"

MainCloudManager::MainCloudManager(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	ui.centralWidget->setLayout(ui.gridLayout);
	stroages.push_back(new YandexDiskManager);
	//stroages.push_back(new OneDriveManager);
	ui.table->setColumnCount(3);
	for(auto i : stroages)
		i->init();
	QFileInfoList files = rootDir.entryInfoList();
	for (auto i : stroages)
		for (auto j : files)
			if (j.isFile())
				i->addFile(j);
	connect(ui.refresh, &QPushButton::clicked, this, &MainCloudManager::refresh);
	connect(ui.remove, &QPushButton::clicked, this, &MainCloudManager::remove);
	connect(ui.sync, &QPushButton::clicked, this, &MainCloudManager::sync);
}

MainCloudManager::~MainCloudManager()
{
	for (auto i : stroages)
		delete i;
}

void MainCloudManager::refresh()
{
	QAbstractManager *i = stroages.first();
	QList<LongName> files = i->managedFiles();
	ui.table->setRowCount(files.size());
	int row = 0;
	for (auto j : files) {
		QTableWidgetItem *name = new QTableWidgetItem(j);
		QTableWidgetItem *ctime = new QTableWidgetItem(i->lastModified(j).toString());
		QTableWidgetItem *ltime = new QTableWidgetItem(QFileInfo(j).lastModified().toString());
		ui.table->setItem(row, 0, name);
		ui.table->setItem(row, 1, ltime);
		ui.table->setItem(row, 2, ctime);
		row++;
	}
}

void MainCloudManager::remove()
{
	int n = ui.table->currentRow();
	QString file = stroages.first()->managedFiles().at(n);
	stroages.first()->removeFile(file);
	refresh();
}

void MainCloudManager::sync()
{
	for (auto i : stroages)
		i->syncAll();
	refresh();
	//stroages.first()->uploadFile(QString("uploadtest.txt"));
}

