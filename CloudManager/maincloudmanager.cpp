#include "maincloudmanager.h"
#include "FileClasses.h"
#include "CommonIncludes.h"
#include "CustomRequestDialog.h"
#include <QDirIterator>

MainCloudManager::MainCloudManager(QWidget *parent)
	: QMainWindow(parent), manager(nullptr)
{
	ui.setupUi(this);
	ui.centralWidget->setLayout(ui.gridLayout);
	ui.table->setColumnCount(5);
	restoreGeometry();
	manager = new CloudManager();	//SLOW 44%
	//stroages.push_back(new YandexDisk("yadisk"));
	//stroages.push_back(new OneDriveManager);
 	//for(auto i : stroages)
 	//	i->init();
	//for (auto i : stroages)

	QDirIterator iter(rootDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
	while (iter.hasNext()) {
		auto info = iter.fileInfo();
		if (info.isFile())
			manager->addFile(info);		
		iter.next();
	}

	connect(this, &QMainWindow::destroyed, qApp, &QApplication::quit);
	connect(ui.refresh, &QPushButton::clicked, this, &MainCloudManager::refresh);
	connect(ui.remove, &QPushButton::clicked, this, &MainCloudManager::remove);
	connect(ui.sync, &QPushButton::clicked, this, &MainCloudManager::sync);
	connect(ui.customRequestButton, &QPushButton::clicked, this, &MainCloudManager::customRequest);
	connect(ui.addCloud, &QPushButton::clicked, this, &MainCloudManager::addCloud);
	connect(ui.removeCloud, &QPushButton::clicked, this, &MainCloudManager::removeCloud);

	refresh();		//SLOW 12%
}

MainCloudManager::~MainCloudManager()
{
	if (manager) delete manager;
	saveGeometry();
	//for (auto i : stroages)
	//	delete i;
}

void MainCloudManager::refresh()
{
	//QAbstractManager *i = stroages.first();
	auto cloudIDs = manager->managedClouds();
	int row = 0;
	ui.table->setRowCount(0);
	for (auto id : cloudIDs) {
		auto cloud = manager->getConstCloudByID(id);
		QString serviceName = cloud->userName() + "@" + cloud->serviceName();
		QSet<LongName> files = cloud->managedFiles();
		ui.table->setRowCount(ui.table->rowCount() + files.size());
		for (auto j : files) {
			QTableWidgetItem *name = new QTableWidgetItem(j);
			QTableWidgetItem *ctime = new QTableWidgetItem(cloud->lastModified(j).toString());		//SLOW 10%
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

void MainCloudManager::remove()		//FIXME remove
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
	CustomRequestDialog dialog(choosenCloud_p, this);
	dialog.show();
	dialog.exec();
}

void MainCloudManager::addCloud()	
{
	QMap<QString, CloudType> cloudTypes;
	cloudTypes["YandexDiskWebDav [DEPRECATED]"] = CloudType::YandexDiskWebDav;
	cloudTypes["YandexDiskREST"] = CloudType::YandexDiskREST;
	cloudTypes["Other"] = CloudType::Other;

	bool ok;
	QString choosenType = QInputDialog::getItem(this, "Custom request", "Choose one of this clouds:", cloudTypes.keys(), 0, false, &ok);
	if (!ok) return;	
	manager->addCloud(cloudTypes[choosenType]);
	refresh();
}

void MainCloudManager::removeCloud()
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



void MainCloudManager::restoreGeometry()
{
	QSettings settings;
	settings.beginGroup("MainWindowGeometry");
	int w = settings.value("WindowWidth", width()).toInt();
	int h = settings.value("WindowHeight", height()).toInt();
	resize(w, h);
	for (int i = 0; i < ui.table->columnCount(); ++i) {
		int w = settings.value("ColumnWidth" + QString::number(i), ui.table->columnWidth(i)).toInt();
		ui.table->setColumnWidth(i, w);
	}
	settings.endGroup();
}

void MainCloudManager::saveGeometry()
{
	QSettings settings;
	settings.beginGroup("MainWindowGeometry");
	settings.setValue("WindowWidth", width());
	settings.setValue("WindowHeight", height());
	for (int i = 0; i < ui.table->columnCount(); ++i) 
		settings.setValue("ColumnWidth" + QString::number(i), ui.table->columnWidth(i));
	settings.endGroup();
}



