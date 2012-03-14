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

namespace Android {
namespace Internal {

class Target;
const char ANDROID_DEPLOYCONFIGURATION_ID[] = "Qt4ProjectManager.AndroidDeployConfiguration";
const char ANDROID_DC_PREFIX[] = "Qt4ProjectManager.AndroidDeployConfiguration.";

class AndroidDeployConfiguration : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT
    friend class AndroidDeployConfigurationFactory;

public:
    AndroidDeployConfiguration(ProjectExplorer::Target *parent);
    virtual ~AndroidDeployConfiguration();
protected:
    AndroidDeployConfiguration(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source);

};

class AndroidDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit AndroidDeployConfigurationFactory(QObject *parent = 0);

    bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) const;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source);

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    // used to translate the ids to names to display to the user
    QString displayNameForId(const QString &id) const;

};

} // namespace Internal
} // namespace Android

#endif // QT4PROJECTMANAGER_QT4ANDROIDDEPLOYCONFIGURATION_H
