#pragma once
#include <QObject>
#include <QThreadPool>
#include "CommonIncludes.h"
#include "AbstractCloud.h"
#include "YandexDisk.h"
#include "YandexDiskREST.h"

enum class CloudType : int { YandexDiskWebDav = 2, YandexDiskREST = 1, Other};
typedef QList<AbstractCloud> CloudList;
typedef int CloudID;
typedef QList<CloudID> CloudIDList;

class CloudManager :
	public QObject
{
	 //CloudList clouds;
	QMap<CloudID, AbstractCloud*> clouds;
	QSet<LongName> allManagedFiles;
	static AbstractCloud* createCloud(CloudType type, const QString& qsettingsGroup = QString());
	static CloudType getCloudType(AbstractCloud* cloud);		//uses RTTI 
public:
	CloudManager();
	CloudID addCloud(CloudType type);
	bool removeCloud(CloudID cloud/*, remove mode*/);
	CloudIDList managedClouds() const;
	const AbstractCloud* getConstCloudByID(CloudID id) { return clouds.contains(id) ? clouds[id]:nullptr; }
	void addFile(const QFileInfo& fileinfo);
	void removeFile(const QFileInfo& fileinfo);
	void syncAll();
	~CloudManager();
};

