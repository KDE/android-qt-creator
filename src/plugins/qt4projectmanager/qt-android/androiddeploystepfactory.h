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
