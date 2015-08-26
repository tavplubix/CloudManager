#include "CloudManager.h"

AbstractCloud* CloudManager::createCloud(CloudType type, const QString& qsettingsGroup /*= QString()*/)
{
	AbstractCloud* newCloud = nullptr;
	switch (type) {
	case CloudType::YandexDisk:
		newCloud = new YandexDisk(qsettingsGroup);
		break;
	default:
		qDebug() << "\n\nERROR: CloudManager::createCloud(): unknown cloudType: cloud wasn't created\n\n";
		return nullptr;
	}
	newCloud->init();
	return newCloud;
}

CloudType CloudManager::getCloudType(AbstractCloud* cloud)
{
	QByteArray className = cloud->metaObject()->className();
	if (className == "YandexDisk") return CloudType::YandexDisk;
	//else if (className == smth) return CloudType::smth;
	else return CloudType::Other;
}

CloudManager::CloudManager()
{
	QSettings settings;
	int size = settings.beginReadArray("Clouds");
	for (int i = 0; i < size; i++) {
		settings.setArrayIndex(i);
		CloudType type = static_cast<CloudType>( settings.value("CloudType", QVariant(static_cast<int>(CloudType::Other))).toInt() );
		CloudID id = settings.value("CloudID", CloudID(0)).toInt();
		QString qsettingsGroup = "CloudGroup" + QString::number(id);
		AbstractCloud* newCloud = createCloud(type, qsettingsGroup);
		clouds[id] = newCloud;
	}
	settings.endArray();
}



CloudID CloudManager::addCloud(CloudType type)
{

	CloudID newID;
	do {
		newID = qrand();
	} while (clouds.contains(newID) || newID == 0);	//OPTIMIZE
	QString qsettingsGroup = "CloudGroup" + QString::number(newID);
	AbstractCloud* newCloud = createCloud(type, qsettingsGroup);
	clouds[newID] = newCloud;
	return newID;
}

bool CloudManager::removeCloud(CloudID cloud)
{
	if (!clouds.contains(cloud)) return false;
	delete clouds[cloud];
	clouds.remove(cloud);
	return true;
}

CloudIDList CloudManager::managedClouds() const
{
	CloudIDList list;
	for (auto iter = clouds.cbegin(); iter != clouds.end(); ++iter) 
		list.push_back(iter.key());
	return std::move(list);
}

CloudManager::~CloudManager()
{
	QSettings settings;
	settings.beginWriteArray("Clouds");
	int i = 0, size = clouds.size();
	for (auto iter = clouds.cbegin(); iter != clouds.end(); ++iter) {		//OPTIMEZE write changes
		settings.setArrayIndex(i++);
		CloudType type = getCloudType(iter.value());
		settings.setValue( "CloudType", QVariant(static_cast<int>(type)) );
		settings.setValue("CloudID", iter.key());
		delete iter.value();
	}
	settings.endArray();
}
