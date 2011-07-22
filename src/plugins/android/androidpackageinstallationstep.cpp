/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidpackageinstallationstep.h"
#include "androidtarget.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/abstractprocessstep.h>


using namespace Android::Internal;
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
    AndroidTarget * androidTarget = qobject_cast<AndroidTarget *>(target());
    if (!androidTarget)
    {
        emit addOutput(tr("Current target is not an android target"), BuildStep::MessageOutput);
        return false;
    }

#ifdef Q_OS_WIN
    // This is here because I hacked QDir::toNativeSeparators to do nothing when __MINGW32__
    // Shouldn't be doing that. If want posix paths under MSYS, I should build runtime shell
    // detection into QtCore.
    QString n = androidTarget->androidDirPath();
    for (int i = 0; i < (int)n.length(); ++i) {
        if (n[i] == QLatin1Char('/'))
            n[i] = QLatin1Char('\\');
    }
    setUserArguments(QString("INSTALL_ROOT=\"%1\" install").arg(n));
#else
    setUserArguments(QString("INSTALL_ROOT=\"%1\" install").arg(androidTarget->androidDirPath()));
#endif

    return MakeStep::init();
}
