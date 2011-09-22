/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidtoolchain.h"
#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androidtarget.h"
#include "androidqtversion.h"

#include "qt4projectmanager/qt4projectmanagerconstants.h"

#include <projectexplorer/gccparser.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <qt4projectmanager/qt4project.h>
#include <qtsupport/qtversionmanager.h>

#include <utils/environment.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

namespace Android {
namespace Internal {

using namespace Qt4ProjectManager;

static const char *const ANDROID_QT_VERSION_KEY = "Qt4ProjectManager.Android.QtVersion";

// --------------------------------------------------------------------------
// MaemoToolChain
// --------------------------------------------------------------------------

AndroidToolChain::AndroidToolChain(bool autodetected) :
    ProjectExplorer::GccToolChain(QLatin1String(Constants::ANDROID_TOOLCHAIN_ID), autodetected),
    m_qtVersionId(-1)
{
    updateId();
}

AndroidToolChain::AndroidToolChain(const AndroidToolChain &tc) :
    ProjectExplorer::GccToolChain(tc),
    m_qtVersionId(tc.m_qtVersionId)
{ }

AndroidToolChain::~AndroidToolChain()
{ }

QString AndroidToolChain::typeName() const
{
    return AndroidToolChainFactory::tr("Android GCC");
}

ProjectExplorer::Abi AndroidToolChain::targetAbi() const
{
    return m_targetAbi;
}

bool AndroidToolChain::isValid() const
{
    return GccToolChain::isValid() && m_qtVersionId >= 0 && m_targetAbi.isValid();
}

void AndroidToolChain::addToEnvironment(Utils::Environment &env) const
{
#ifdef __GNUC__
#warning TODO this vars should be configurable in projects -> build tab
#warning TODO invalidate all .pro files !!!
#endif
    QString ndk_host = QLatin1String(
#if defined(Q_OS_LINUX)
        "linux-x86"
#elif defined(Q_OS_WIN)
        "windows"
#elif defined(Q_OS_MAC)
        "darwin-x86"
#endif
    );

    // this env vars are used by qmake mkspecs to generate makefiles (check QTDIR/mkspecs/android-g++/qmake.conf for more info)
    env.set(QLatin1String("ANDROID_NDK_HOST")
                     ,ndk_host);
    env.set(QLatin1String("ANDROID_NDK_ROOT")
                     ,QDir::toNativeSeparators(AndroidConfigurations::instance().config().NDKLocation));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_PREFIX"), AndroidConfigurations::toolchainPrefix(m_targetAbi.architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLS_PREFIX"), AndroidConfigurations::toolsPrefix(m_targetAbi.architecture()));
    env.set(QLatin1String("ANDROID_NDK_TOOLCHAIN_VERSION"),AndroidConfigurations::instance().config().NDKToolchainVersion);
#if defined(Q_OS_WIN)
    // These fixes are needed so that make.exe gets found by Environment::searchInPath()
    // Seems '.' gets added to the path instead of argv[0] sometimes, which fails
    // after having changed to the build directory when we look for "make"
    // To fix this I force applicationDirPath to the front of PATH.
    env.prependOrSet("PATH", QCoreApplication::applicationDirPath(), ";");
    // This is a convenience thing:
    // When debugging Necessitas QtCreator through QtCreator, PATHEXT isn't passed through. Another
    // fix for this would be to find out where the arg passing happens when using "Start and Debug
    // External Application" and fix that, I do it this way because I debug Necessitas QtCreator with
    // the latest official Nokia QtCreator; can't use qgetenv("PATHEXT") either as it's already dead.
    env.set("PATHEXT", ".COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH;.MSC");
#endif

    Qt4Project *qt4pro = qobject_cast<Qt4Project *>(ProjectExplorer::ProjectExplorerPlugin::instance()->currentProject());
    if (!qt4pro)
        return;
    AndroidTarget * at = qobject_cast<AndroidTarget *>(qt4pro->activeTarget());
    if (!at)
        return;
    env.set(QLatin1String("ANDROID_NDK_PLATFORM")
                     ,AndroidConfigurations::instance().bestMatch(at->targetSDK()));
}

QString AndroidToolChain::sysroot() const
{
//    QtVersion *v = QtVersionManager::instance()->version(m_qtVersionId);
//    if (!v)
        return QString();

//    if (m_sysroot.isEmpty()) {
//        QFile file(QDir::cleanPath(MaemoGlobal::targetRoot(v)) + QLatin1String("/information"));
//        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//            QTextStream stream(&file);
//            while (!stream.atEnd()) {
//                const QString &line = stream.readLine().trimmed();
//                const QStringList &list = line.split(QLatin1Char(' '));
//                if (list.count() > 1 && list.at(0) == QLatin1String("sysroot"))
//                    m_sysroot = MaemoGlobal::maddeRoot(v) + QLatin1String("/sysroots/") + list.at(1);
//            }
//        }
//    }
//    return m_sysroot;
}

bool AndroidToolChain::operator ==(const ProjectExplorer::ToolChain &tc) const
{
    if (!ToolChain::operator ==(tc))
        return false;

    const AndroidToolChain *tcPtr = static_cast<const AndroidToolChain *>(&tc);
    return m_qtVersionId == tcPtr->m_qtVersionId;
}

ProjectExplorer::ToolChainConfigWidget *AndroidToolChain::configurationWidget()
{
    return new AndroidToolChainConfigWidget(this);
}

QVariantMap AndroidToolChain::toMap() const
{
    QVariantMap result = GccToolChain::toMap();
    result.insert(QLatin1String(ANDROID_QT_VERSION_KEY), m_qtVersionId);
    return result;
}

bool AndroidToolChain::fromMap(const QVariantMap &data)
{
    if (!GccToolChain::fromMap(data))
        return false;

    m_qtVersionId = data.value(QLatin1String(ANDROID_QT_VERSION_KEY), -1).toInt();

    return isValid();
}

QString AndroidToolChain::mkspec() const
{
    return "android-g++";
}

void AndroidToolChain::setQtVersionId(int id)
{
    if (id < 0) {
        m_targetAbi = ProjectExplorer::Abi();
        m_qtVersionId = -1;
        updateId();
        return;
    }

    QtSupport::BaseQtVersion *version = QtSupport::QtVersionManager::instance()->version(id);
    Q_ASSERT(version);
    m_qtVersionId = id;

    Q_ASSERT(version->qtAbis().count() == 1);
    m_targetAbi = version->qtAbis().at(0);

    updateId();
    setDisplayName(AndroidToolChainFactory::tr("Android Gcc for %1").arg(version->displayName()));
}

int AndroidToolChain::qtVersionId() const
{
    return m_qtVersionId;
}

QList<ProjectExplorer::Abi> AndroidToolChain::detectSupportedAbis() const
{
    return QList<ProjectExplorer::Abi>()<<m_targetAbi;
}

void AndroidToolChain::updateId()
{
    setId(QString::fromLatin1("%1:%2").arg(Constants::ANDROID_TOOLCHAIN_ID).arg(m_qtVersionId));
}

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

AndroidToolChainConfigWidget::AndroidToolChainConfigWidget(AndroidToolChain *tc) :
   ProjectExplorer::ToolChainConfigWidget(tc)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel;
    QtSupport::BaseQtVersion *v = QtSupport::QtVersionManager::instance()->version(tc->qtVersionId());
    Q_ASSERT(v);
    label->setText(tr("NDK Root: %1").arg(AndroidConfigurations::instance().config().NDKLocation));
    layout->addWidget(label);
}

void AndroidToolChainConfigWidget::apply()
{
    // nothing to do!
}

void AndroidToolChainConfigWidget::discard()
{
    // nothing to do!
}

bool AndroidToolChainConfigWidget::isDirty() const
{
    return false;
}

// --------------------------------------------------------------------------
// ToolChainFactory
// --------------------------------------------------------------------------

AndroidToolChainFactory::AndroidToolChainFactory() :
    ProjectExplorer::ToolChainFactory()
{ }

QString AndroidToolChainFactory::displayName() const
{
    return tr("Android GCC");
}

QString AndroidToolChainFactory::id() const
{
    return QLatin1String(Constants::ANDROID_TOOLCHAIN_ID);
}

QList<ProjectExplorer::ToolChain *> AndroidToolChainFactory::autoDetect()
{
    QList<ProjectExplorer::ToolChain *> result;

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    connect(vm, SIGNAL(qtVersionsChanged(QList<int>)),
            this, SLOT(handleQtVersionChanges(QList<int>)));

    QList<int> versionList;
    foreach (QtSupport::BaseQtVersion *v, vm->versions())
        versionList.append(v->uniqueId());

    return createToolChainList(versionList);
}

void AndroidToolChainFactory::handleQtVersionChanges(const QList<int> &changes)
{
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();
    QList<ProjectExplorer::ToolChain *> tcList = createToolChainList(changes);
    foreach (ProjectExplorer::ToolChain *tc, tcList)
        tcm->registerToolChain(tc);
}

QList<ProjectExplorer::ToolChain *> AndroidToolChainFactory::createToolChainList(const QList<int> &changes)
{
    ProjectExplorer::ToolChainManager *tcm = ProjectExplorer::ToolChainManager::instance();
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    QList<ProjectExplorer::ToolChain *> result;

    foreach (int i, changes) {
        QtSupport::BaseQtVersion *v = vm->version(i);
        QList<ProjectExplorer::ToolChain *> toRemove;
        foreach (ProjectExplorer::ToolChain *tc, tcm->toolChains()) {
            if (tc->id()!=QLatin1String(Constants::ANDROID_TOOLCHAIN_ID))
                continue;
            AndroidToolChain *aTc = static_cast<AndroidToolChain *>(tc);
            if (aTc->qtVersionId() == i)
                toRemove.append(aTc);
        }
        foreach (ProjectExplorer::ToolChain *tc, toRemove)
            tcm->deregisterToolChain(tc);

        const AndroidQtVersion * const aqv = dynamic_cast<AndroidQtVersion *>(v);
        if (!aqv || !aqv->isValid())
            continue;

        AndroidToolChain *aTc = new AndroidToolChain(true);
        aTc->setQtVersionId(i);
        aTc->setDisplayName(tr("Android GCC (%1-%2)")
                            .arg(ProjectExplorer::Abi::toString(aTc->targetAbi().architecture()))
                            .arg(AndroidConfigurations::instance().config().NDKToolchainVersion));
        aTc->setCompilerPath(AndroidConfigurations::instance().gccPath(aTc->targetAbi().architecture()));
        result.append(aTc);
    }
    return result;
}

} // namespace Internal
} // namespace Android
