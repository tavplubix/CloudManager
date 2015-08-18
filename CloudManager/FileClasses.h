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
class ItsDir : public QException {};

class ShortName {
	//Q_OBJECT
	QString shortName;
public:
	ShortName(const QString& s);
	ShortName(const LongName& ln);
	//ShortName(const File& fi);
	operator QString() const { return shortName; };
};

class LongName {
	//Q_OBJECT
	QString longName;
public:
	LongName(const QString& s);
	LongName(const ShortName& sn);
	//ShortName(const File& fi);
	operator QString() const { return longName; };
};



