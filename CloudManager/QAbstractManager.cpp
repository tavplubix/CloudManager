#include "QAbstractManager.h"


QAbstractManager::QAbstractManager()
{
}


QAbstractManager::Status QAbstractManager::currentStatus()
{
	return status;
}

void QAbstractManager::setActionWithChanged(ActionWithChanged act)
{
	action = act;
}

void QAbstractManager::syncAll()
{
	/*downloadAllNew();		//WARNING
	uploadAllNew();			//WARNING
	switch (action) {
	case ActionWithChanged::Pull :
		pullChangedFiles();
		break;
	case ActionWithChanged::Push :
		pushChangedFiles();
		break;
	}*/
}

QAbstractManager::~QAbstractManager()
{
}
