#include "maincloudmanager.h"
#include "CommonIncludes.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainCloudManager w;
	w.show();
	return a.exec();
}
