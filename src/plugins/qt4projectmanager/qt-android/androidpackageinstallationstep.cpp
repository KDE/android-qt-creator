#include "androidpackageinstallationstep.h"
#include "qt4androidtarget.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/abstractprocessstep.h>


using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
const QLatin1String AndroidPackageInstallationStep::Id("Qt4ProjectManager.AndroidPackageInstallationStep");

AndroidPackageInstallationStep::AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bsl) : MakeStep(bsl, Id)
{
    setDefaultDisplayName(tr("Copy application data"));
    setDisplayName(tr("Copy application data"));
}

AndroidPackageInstallationStep::AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bc, AndroidPackageInstallationStep *other): MakeStep(bc, other)
{
    setDefaultDisplayName(tr("Copy application data"));
    setDisplayName(tr("Copy application data"));
}

AndroidPackageInstallationStep::~AndroidPackageInstallationStep()
{
}

bool AndroidPackageInstallationStep::init()
{
    Qt4AndroidTarget * androidTarget = qobject_cast<Qt4AndroidTarget *>(target());
    if (!androidTarget)
    {
        emit addOutput(tr("Current target is not an android target"), BuildStep::MessageOutput);
        return false;
    }
    setUserArguments(QString("INSTALL_ROOT=\"%1\" install").arg(androidTarget->androidDirPath()));
    return MakeStep::init();
}
