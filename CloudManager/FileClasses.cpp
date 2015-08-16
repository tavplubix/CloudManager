#include "FileClasses.h"




ShortName::ShortName(const QString& s)
{
	//TODO обрабатывать файлы не в rootDir
	QFileInfo file(s);
	if (file.isRelative()) {
		file = rootDir.absoluteFilePath(s);
		if (!file.exists())
			//throw FileDoesNotExist();
			qDebug() << "WARNING: File does not exist\n";
		if (file.isDir()) throw ItsDir();
		else shortName = s;
	}
	else {
		if (!file.exists())
			//throw FileDoesNotExist();
			qDebug() << "WARNING: File does not exist\n";
		if (file.isDir()) throw ItsDir();
		QString rel = rootDir.relativeFilePath(s);
		if (rel.contains("..") || rel.isEmpty()) throw NotImplemented();		
		else shortName = rel;
	}
}

LongName::LongName(const QString& s)
{
	QFileInfo file(s);
	if (file.isAbsolute()) {
		if (!file.exists())
			//throw FileDoesNotExist();
			qDebug() << "WARNING: File does not exist\n";
		if (!file.absoluteFilePath().contains(rootDir.absolutePath())) throw NotImplemented();
		if (file.isDir()) throw ItsDir();
		else longName = s;
	}
	else {
		file = rootDir.absoluteFilePath(s);
		if (!file.exists())
			//throw FileDoesNotExist();
			qDebug() << "WARNING: File does not exist\n";
		if (file.isDir()) throw ItsDir();
		else longName = file.absoluteFilePath();
	}

}
