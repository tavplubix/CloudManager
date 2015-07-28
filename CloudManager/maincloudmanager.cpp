#include "maincloudmanager.h"

MainCloudManager::MainCloudManager(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	ui.centralWidget->setLayout(ui.gridLayout);
	connect(ui.chooseb, &QPushButton::clicked, this, &MainCloudManager::choose);
	connect(ui.uploadb, &QPushButton::clicked, this, &MainCloudManager::upload);
	connect(ui.downloadb, &QPushButton::clicked, this, &MainCloudManager::download);

}

MainCloudManager::~MainCloudManager()
{

}

void MainCloudManager::choose()
{
	QString f = QFileDialog::getOpenFileName();
	ui.lineEdit->setText(f);
}

void MainCloudManager::download()
{
	QFileInfo f = ui.lineEdit->text();
	disk.downloadFile(f);
}

void MainCloudManager::upload()
{
	QFileInfo f = ui.lineEdit->text();
	disk.uploadFile(f);
}
