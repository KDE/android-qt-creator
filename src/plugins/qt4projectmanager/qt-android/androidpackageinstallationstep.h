#ifndef ANDROIDPACKAGEINSTALLATIONSTEP_H
#define ANDROIDPACKAGEINSTALLATIONSTEP_H

#include <qt4projectmanager/makestep.h>
namespace Qt4ProjectManager {
namespace Internal {

class AndroidPackageInstallationStep : public MakeStep
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
} // namespace Qt4ProjectManager

#endif // ANDROIDPACKAGEINSTALLATIONSTEP_H
