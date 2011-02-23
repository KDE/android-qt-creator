/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef QT4PROJECTMANAGER_QT4ANDROIDDEPLOYCONFIGURATION_H
#define QT4PROJECTMANAGER_QT4ANDROIDDEPLOYCONFIGURATION_H

#include <projectexplorer/deployconfiguration.h>

namespace Qt4ProjectManager {
namespace Internal {

class Target;

class Qt4AndroidDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit Qt4AndroidDeployConfigurationFactory(QObject *parent = 0);

    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
//    if (id == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
//        return QApplication::translate("Qt4ProjectManager::Internal::Qt4Target", "Android", "Qt4 Android target display name");
//    if (id == QLatin1String(Qt4ProjectManager::Constants::ANDROID_DEVICE_TARGET_ID))
//        return QIcon(QLatin1String(":/projectexplorer/images/AndroidDevice.png"));
//    else if (id() == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
//        addRunConfiguration(new AndroidRunConfiguration(this, proFilePath));
//    else if (id == QLatin1String(Constants::ANDROID_DEVICE_TARGET_ID))
//        shortName = QLatin1String("android");

};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4PROJECTMANAGER_QT4ANDROIDDEPLOYCONFIGURATION_H
