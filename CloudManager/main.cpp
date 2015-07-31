#include "maincloudmanager.h"
#include "CommonIncludes.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainCloudManager w;
	w.show();
	try {
		return a.exec();
	}
	catch (...) {
		qDebug() << QString::fromLocal8Bit("exception");
	}
	return 0;
}
