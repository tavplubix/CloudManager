#include "maincloudmanager.h"
#include "FileClasses.h"
#include "CommonIncludes.h"
#include "CustomRequestDialog.h"

MainCloudManager::MainCloudManager(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	ui.centralWidget->setLayout(ui.gridLayout);
	//manager = new CloudManager();
	stroages.push_back(new YandexDisk("yadisk"));
	//stroages.push_back(new OneDriveManager);
	ui.table->setColumnCount(5);
 	for(auto i : stroages)
 		i->init();
	QFileInfoList files = rootDir.entryInfoList();
	for (auto i : stroages)
		for (auto j : files)
			if (j.isFile())
				i->addFile(j);
	connect(this, &QMainWindow::destroyed, qApp, &QApplication::quit);
	connect(ui.refresh, &QPushButton::clicked, this, &MainCloudManager::refresh);
	connect(ui.remove, &QPushButton::clicked, this, &MainCloudManager::remove);
	connect(ui.sync, &QPushButton::clicked, this, &MainCloudManager::sync);
	connect(ui.customRequestButton, &QPushButton::clicked, this, &MainCloudManager::customRequest);
	connect(ui.addCloud, &QPushButton::clicked, this, &MainCloudManager::addCloud);
	connect(ui.removeCloud, &QPushButton::clicked, this, &MainCloudManager::removeCloud);
}

MainCloudManager::~MainCloudManager()
{
	//if (manager) delete manager;
	for (auto i : stroages)
		delete i;
}

void MainCloudManager::refresh()
{
	//QAbstractManager *i = stroages.first();
	for (auto i : stroages) {
		QString serviceName = i->userName() + "@" + i->serviceName();
		QList<LongName> files = i->managedFiles();
		ui.table->setRowCount(files.size());
		int row = 0;
		for (auto j : files) {
			QTableWidgetItem *name = new QTableWidgetItem(j);
			QTableWidgetItem *ctime = new QTableWidgetItem(i->lastModified(j).toString());
			QTableWidgetItem *ltime = new QTableWidgetItem(QFileInfo(j).lastModified().toString());
			QTableWidgetItem *service = new QTableWidgetItem(serviceName);
			ui.table->setItem(row, 0, name);
			ui.table->setItem(row, 1, ltime);
			ui.table->setItem(row, 2, ctime);
			ui.table->setItem(row, 3, service);
			row++;
		}
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

void MainCloudManager::customRequest()
{
	auto dialog = CustomRequestDialog(stroages.first(), this);
	dialog.show();
	dialog.exec();
}

void MainCloudManager::addCloud()
{


}

void MainCloudManager::removeCloud()
{

}

