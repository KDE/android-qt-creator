#ifndef ANDROIDPACKAGEINSTALLATIONSTEP_H
#define ANDROIDPACKAGEINSTALLATIONSTEP_H

#include <qt4projectmanager/makestep.h>

namespace Android {
namespace Internal {

class AndroidPackageInstallationStep : public Qt4ProjectManager::MakeStep
{
    friend class AndroidPackageInstallationFactory;
public:
    explicit AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bsl);
    virtual ~AndroidPackageInstallationStep();
    virtual bool init();
private:
    AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bc,
        AndroidPackageInstallationStep *other);

private:
    static const QLatin1String Id;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDPACKAGEINSTALLATIONSTEP_H
