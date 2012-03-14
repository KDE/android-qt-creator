/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

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
