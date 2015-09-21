#include "FileClasses.h"



//UNDONE process any files (not only F:/Backups/CloudManager/*)

ShortName::ShortName(const QString& s)
{
	qInfo("ShortName(const QString& s)");
	QDir rootDir = "F:/Backups/CloudManager/";	//Hardcoded temporary
	QFileInfo file(s);
	if (file.isRelative()) {
		auto tmp = rootDir.absolutePath();
		file = rootDir.absoluteFilePath(s);		
		if (!file.exists())
			//throw FileDoesNotExist();
			qDebug() << "WARNING: File " + s + " does not exist\n";
		//if (file.isDir()) throw ItsNotFile();
		shortName = s;
	}
	else {
		if (!file.exists())
			//throw FileDoesNotExist();
			qDebug() << "WARNING: File " + s + " does not exist\n";
		//if (file.isDir()) throw ItsNotFile();
		QString rel = rootDir.relativeFilePath(s);
		if (rel.contains("..") || rel.isEmpty()) throw NotImplemented();		
		else shortName = rel;
	}
}

ShortName::ShortName(const LongName& ln)
	: ShortName((QString)ln) {}

ShortName::ShortName(const QFileInfo& qfi)
	: ShortName(qfi.absoluteFilePath())		//FIXME what if it's not absolute?
{
}

LongName::LongName(const QString& s)
{
	qInfo("LongName(const QString& s)");
	QDir rootDir = "F:/Backups/CloudManager/";	//Hardcoded temporary
	QFileInfo file(s);
	if (file.isAbsolute()) {
		if (!file.exists())
			//throw FileDoesNotExist();
			qDebug() << "WARNING: File " + s + " does not exist\n";
		if (!file.absoluteFilePath().contains(rootDir.absolutePath())) throw NotImplemented();
		//if (file.isDir()) throw ItsNotFile();
		else longName = s;
	}
	else {
		file = rootDir.absoluteFilePath(s);
		if (!file.exists())
			//throw FileDoesNotExist();
			qDebug() << "WARNING: File " + s + " does not exist\n";
		//if (file.isDir()) throw ItsNotFile();
		longName = file.absoluteFilePath();
	}

}

LongName::LongName(const ShortName& sn)
	: LongName( (QString)sn ) {}

LongName::LongName(const QFileInfo& qfi)
	: LongName(qfi.absoluteFilePath())		//FIXME what if it's not absolute?
{
}
