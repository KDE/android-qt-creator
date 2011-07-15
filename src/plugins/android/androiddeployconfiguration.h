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

class AndroidDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit AndroidDeployConfigurationFactory(QObject *parent = 0);

    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
};

} // namespace Internal
} // namespace Android

#endif // QT4PROJECTMANAGER_QT4ANDROIDDEPLOYCONFIGURATION_H
