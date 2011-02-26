/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDDEPLOYSTEPFACTORY_H
#define ANDROIDDEPLOYSTEPFACTORY_H

#include <projectexplorer/buildstep.h>

namespace Qt4ProjectManager {
namespace Internal {

class AndroidDeployStepFactory : public ProjectExplorer::IBuildStepFactory
{
public:
    AndroidDeployStepFactory(QObject *parent);

    virtual QStringList availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    virtual QString displayNameForId(const QString &id) const;

    virtual bool canCreate(ProjectExplorer::BuildStepList *parent,
                           const QString &id) const;
    virtual ProjectExplorer::BuildStep *
            create(ProjectExplorer::BuildStepList *parent, const QString &id);

    virtual bool canRestore(ProjectExplorer::BuildStepList *parent,
                            const QVariantMap &map) const;
    virtual ProjectExplorer::BuildStep *
            restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    virtual bool canClone(ProjectExplorer::BuildStepList *parent,
                          ProjectExplorer::BuildStep *product) const;
    virtual ProjectExplorer::BuildStep *
            clone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *product);
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDDEPLOYSTEPFACTORY_H
