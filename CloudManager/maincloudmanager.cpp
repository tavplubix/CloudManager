#include "maincloudmanager.h"
#include "FileClasses.h"
#include "CommonIncludes.h"
#include "CustomRequestDialog.h"

MainCloudManager::MainCloudManager(QWidget *parent)
	: QMainWindow(parent), manager(nullptr)
{
	ui.setupUi(this);
	ui.centralWidget->setLayout(ui.gridLayout);
	manager = new CloudManager();
	//stroages.push_back(new YandexDisk("yadisk"));
	//stroages.push_back(new OneDriveManager);
	ui.table->setColumnCount(5);
 	//for(auto i : stroages)
 	//	i->init();
	QFileInfoList files = rootDir.entryInfoList();
	//for (auto i : stroages)
		for (auto j : files)
			if (j.isFile())
				manager->addFile(j);
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
	if (manager) delete manager;
	//for (auto i : stroages)
	//	delete i;
}

void MainCloudManager::refresh()
{
	//QAbstractManager *i = stroages.first();
	auto cloudIDs = manager->managedClouds();
	for (auto id : cloudIDs) {
		auto cloud = manager->getConstCloudByID(id);
		QString serviceName = cloud->userName() + "@" + cloud->serviceName();
		QList<LongName> files = cloud->managedFiles();
		ui.table->setRowCount(files.size());
		int row = 0;
		for (auto j : files) {
			QTableWidgetItem *name = new QTableWidgetItem(j);
			QTableWidgetItem *ctime = new QTableWidgetItem(cloud->lastModified(j).toString());
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
	auto item = ui.table->item(n, 0);
	QString file = item->text();
	manager->removeFile(file);
	refresh();
}

void MainCloudManager::sync()
{
	QFileInfoList files = rootDir.entryInfoList();
	for (auto file : files)
		if (file.isFile())
			manager->addFile(file);
	manager->syncAll();
	refresh();
}

void MainCloudManager::customRequest()
{
	QStringList list;
	QMap<QString, CloudID> clouds;
	auto cloudIDs = manager->managedClouds();
	for (auto id : cloudIDs) {
		auto cloud = manager->getConstCloudByID(id);
		QString serviceName = cloud->userName() + "@" + cloud->serviceName();
		list.push_back(serviceName);
		clouds[serviceName] = id;
	}
	bool ok;
	QString choosenCloud = QInputDialog::getItem(this, "Custom request", "Choose one of this clouds:", list, 0, false, &ok);
	if (!ok) return;
	auto choosenCloud_cp = manager->getConstCloudByID(clouds[choosenCloud]);
	auto choosenCloud_p = const_cast<AbstractCloud*>(choosenCloud_cp);
	auto dialog = CustomRequestDialog(choosenCloud_p, this);
	dialog.show();
	dialog.exec();
}

void MainCloudManager::addCloud()	//FIXME addCloud()
{
	QMap<QString, CloudType> cloudTypes;
	cloudTypes["YandexDisk"] = CloudType::YandexDisk;
	cloudTypes["Other"] = CloudType::Other;

	bool ok;
	QString choosenType = QInputDialog::getItem(this, "Custom request", "Choose one of this clouds:", cloudTypes.keys(), 0, false, &ok);
	if (!ok) return;	
	manager->addCloud(cloudTypes[choosenType]);
	refresh();
}

void MainCloudManager::removeCloud()	//FIXME removeCloud()
{
	QStringList list;
	QMap<QString, CloudID> clouds;
	auto cloudIDs = manager->managedClouds();
	for (auto id : cloudIDs) {
		auto cloud = manager->getConstCloudByID(id);
		QString serviceName = cloud->userName() + "@" + cloud->serviceName();
		list.push_back(serviceName);
		clouds[serviceName] = id;
	}
	bool ok;
	QString choosenCloud = QInputDialog::getItem(this, "Custom request", "Choose one of this clouds:", list, 0, false, &ok);
	if (!ok) return;
	//if (manager->managedClouds().isEmpty()) return;	
	manager->removeCloud(clouds[choosenCloud]);
	refresh();
}

