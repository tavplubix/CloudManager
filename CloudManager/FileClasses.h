#pragma once
#include "CommonIncludes.h"

class ShortName;
class LongName;
class File;
class FileID;


typedef QSet<ShortName> ShortNameSet;
typedef QSet<LongName> LongNameSet;


class FileDoesNotExist : public QException {};
class NotImplemented : public QException {};
class ItsNotFile : public QException {};
class ItsNotDir : public QException {};

class ShortName {
	//Q_OBJECT
	QString shortName;
public:
	ShortName(const QString& s);
	ShortName(const LongName& ln);
	ShortName(const QFileInfo& qfi);
	//ShortName(const File& fi);
	operator QString() const { return shortName; };
};

class LongName {
	//Q_OBJECT
	QString longName;
public:
	LongName(const QString& s);
	LongName(const ShortName& sn);
	LongName(const QFileInfo& qfi);
	//ShortName(const File& fi);
	operator QString() const { return longName; };
};



