#include "maincloudmanager.h"
//#include "CommonIncludes.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("tavplubix");
	QCoreApplication::setApplicationName("CloudManager");
	QApplication a(argc, argv);
	MainCloudManager w;
	w.show();
	try {
		return a.exec();
	}
	catch (...) {
		qInfo("EXCEPTION");
		throw;
	}
	return 0;
}
