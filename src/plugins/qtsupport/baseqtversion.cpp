/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "baseqtversion.h"
#include "qmlobservertool.h"
#include "qmldumptool.h"
#include "qmldebugginglibrary.h"

#include "qtversionmanager.h"
#include "profilereader.h"
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/debugginghelper.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/persistentsettings.h>
#include <utils/environment.h>
#include <utils/synchronousprocess.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

using namespace QtSupport;
using namespace QtSupport::Internal;

static const char QTVERSIONID[] = "Id";
static const char QTVERSIONNAME[] = "Name";
static const char QTVERSIONAUTODETECTED[] = "isAutodetected";
static const char QTVERSIONAUTODETECTIONSOURCE []= "autodetectionSource";
static const char QTVERSIONQMAKEPATH[] = "QMakePath";

///////////////
// QtVersionNumber
///////////////
QtVersionNumber::QtVersionNumber(int ma, int mi, int p)
    : majorVersion(ma), minorVersion(mi), patchVersion(p)
{
}

QtVersionNumber::QtVersionNumber(const QString &versionString)
{
    if (!checkVersionString(versionString)) {
        majorVersion = minorVersion = patchVersion = -1;
        return;
    }

    QStringList parts = versionString.split(QLatin1Char('.'));
    majorVersion = parts.at(0).toInt();
    minorVersion = parts.at(1).toInt();
    patchVersion = parts.at(2).toInt();
}

QtVersionNumber::QtVersionNumber()
{
    majorVersion = minorVersion = patchVersion = -1;
}

bool QtVersionNumber::checkVersionString(const QString &version) const
{
    int dots = 0;
    const QString validChars = QLatin1String("0123456789.");
    foreach (const QChar &c, version) {
        if (!validChars.contains(c))
            return false;
        if (c == QLatin1Char('.'))
            ++dots;
    }
    if (dots != 2)
        return false;
    return true;
}

bool QtVersionNumber::operator <(const QtVersionNumber &b) const
{
    if (majorVersion < b.majorVersion)
        return true;
    if (majorVersion > b.majorVersion)
        return false;
    if (minorVersion < b.minorVersion)
        return true;
    if (minorVersion > b.minorVersion)
        return false;
    if (patchVersion < b.patchVersion)
        return true;
    return false;
}

bool QtVersionNumber::operator >(const QtVersionNumber &b) const
{
    return b < *this;
}

bool QtVersionNumber::operator ==(const QtVersionNumber &b) const
{
    return majorVersion == b.majorVersion
            && minorVersion == b.minorVersion
            && patchVersion == b.patchVersion;
}

bool QtVersionNumber::operator !=(const QtVersionNumber &b) const
{
    return !(*this == b);
}

bool QtVersionNumber::operator <=(const QtVersionNumber &b) const
{
    return !(*this > b);
}

bool QtVersionNumber::operator >=(const QtVersionNumber &b) const
{
    return b <= *this;
}

///////////////
// QtConfigWidget
///////////////
QtConfigWidget::QtConfigWidget()
{

}

///////////////
// BaseQtVersion
///////////////
int BaseQtVersion::getUniqueId()
{
    return QtVersionManager::instance()->getUniqueId();
}

BaseQtVersion::BaseQtVersion(const Utils::FileName &qmakeCommand, bool isAutodetected, const QString &autodetectionSource)
    : m_id(getUniqueId()),
      m_isAutodetected(isAutodetected),
      m_autodetectionSource(autodetectionSource),
      m_hasDebuggingHelper(false),
      m_hasQmlDump(false),
      m_hasQmlDebuggingLibrary(false),
      m_hasQmlObserver(false),
      m_mkspecUpToDate(false),
      m_mkspecReadUpToDate(false),
      m_defaultConfigIsDebug(true),
      m_defaultConfigIsDebugAndRelease(true),
      m_versionInfoUpToDate(false),
      m_installed(true),
      m_hasExamples(false),
      m_hasDemos(false),
      m_hasDocumentation(false),
      m_qmakeIsExecutable(true)
{
    ctor(qmakeCommand);
    setDisplayName(defaultDisplayName(qtVersionString(), qmakeCommand, false));
}

BaseQtVersion::BaseQtVersion()
    :  m_id(-1), m_isAutodetected(false),
    m_hasDebuggingHelper(false),
    m_hasQmlDump(false),
    m_hasQmlDebuggingLibrary(false),
    m_hasQmlObserver(false),
    m_mkspecUpToDate(false),
    m_mkspecReadUpToDate(false),
    m_defaultConfigIsDebug(true),
    m_defaultConfigIsDebugAndRelease(true),
    m_versionInfoUpToDate(false),
    m_installed(true),
    m_hasExamples(false),
    m_hasDemos(false),
    m_hasDocumentation(false),
    m_qmakeIsExecutable(true)
{
    ctor(Utils::FileName());
}

void BaseQtVersion::ctor(const Utils::FileName &qmakePath)
{
    m_qmakeCommand = qmakePath;
    m_designerCommand.clear();
    m_linguistCommand.clear();
    m_qmlviewerCommand.clear();
    m_uicCommand.clear();
    m_mkspecUpToDate = false;
    m_mkspecReadUpToDate = false;
    m_versionInfoUpToDate = false;
    m_qtVersionString.clear();
    m_sourcePath.clear();
}

BaseQtVersion::~BaseQtVersion()
{
}

QString BaseQtVersion::defaultDisplayName(const QString &versionString, const Utils::FileName &qmakePath,
                                          bool fromPath)
{
    QString location;
    if (qmakePath.isEmpty()) {
        location = QCoreApplication::translate("QtVersion", "<unknown>");
    } else {
        // Deduce a description from '/foo/qt-folder/[qtbase]/bin/qmake' -> '/foo/qt-folder'.
        // '/usr' indicates System Qt 4.X on Linux.
        QDir dir = qmakePath.toFileInfo().absoluteDir();
        do {
            const QString dirName = dir.dirName();
            if (dirName == QLatin1String("usr")) { // System-installed Qt.
                location = QCoreApplication::translate("QtVersion", "System");
                break;
            }
            if (dirName.compare(QLatin1String("bin"), Qt::CaseInsensitive)
                && dirName.compare(QLatin1String("qtbase"), Qt::CaseInsensitive)) {
                location = dirName;
                break;
            }
        } while (dir.cdUp());
    }

    return fromPath ?
        QCoreApplication::translate("QtVersion", "Qt %1 in PATH (%2)").arg(versionString, location) :
        QCoreApplication::translate("QtVersion", "Qt %1 (%2)").arg(versionString, location);
}

void BaseQtVersion::setId(int id)
{
    m_id = id;
}

void BaseQtVersion::restoreLegacySettings(QSettings *s)
{
    Q_UNUSED(s);
}

void BaseQtVersion::fromMap(const QVariantMap &map)
{
    m_id = map.value(QLatin1String(QTVERSIONID)).toInt();
    if (m_id == -1) // this happens on adding from installer, see updateFromInstaller => get a new unique id
        m_id = QtVersionManager::instance()->getUniqueId();
    m_displayName = map.value(QLatin1String(QTVERSIONNAME)).toString();
    m_isAutodetected = map.value(QLatin1String(QTVERSIONAUTODETECTED)).toBool();
    if (m_isAutodetected)
        m_autodetectionSource = map.value(QLatin1String(QTVERSIONAUTODETECTIONSOURCE)).toString();
    QString string = map.value(QLatin1String(QTVERSIONQMAKEPATH)).toString();
    if (string.startsWith(QLatin1Char('~')))
        string.remove(0, 1).prepend(QDir::homePath());
    ctor(Utils::FileName::fromUserInput(string));
}

QVariantMap BaseQtVersion::toMap() const
{
    QVariantMap result;
    result.insert(QLatin1String(QTVERSIONID), uniqueId());
    result.insert(QLatin1String(QTVERSIONNAME), displayName());
    result.insert(QLatin1String(QTVERSIONAUTODETECTED), isAutodetected());
    if (isAutodetected())
        result.insert(QLatin1String(QTVERSIONAUTODETECTIONSOURCE), autodetectionSource());
    result.insert(QLatin1String(QTVERSIONQMAKEPATH), qmakeCommand().toString());
    return result;
}

bool BaseQtVersion::isValid() const
{
    if (uniqueId() == -1 || displayName().isEmpty())
        return false;
    updateVersionInfo();
    updateMkspec();

    return  !qmakeCommand().isEmpty()
            && m_installed
            && m_versionInfo.contains(QLatin1String("QT_INSTALL_BINS"))
            && !m_mkspecFullPath.isEmpty()
            && m_qmakeIsExecutable;
}

QString BaseQtVersion::invalidReason() const
{
    if (displayName().isEmpty())
        return QCoreApplication::translate("QtVersion", "Qt version has no name");
    if (qmakeCommand().isEmpty())
        return QCoreApplication::translate("QtVersion", "No qmake path set");
    if (!m_qmakeIsExecutable)
        return QCoreApplication::translate("QtVersion", "qmake does not exist or is not executable");
    if (!m_installed)
        return QCoreApplication::translate("QtVersion", "Qt version is not properly installed, please run make install");
    if (!m_versionInfo.contains(QLatin1String("QT_INSTALL_BINS")))
        return QCoreApplication::translate("QtVersion",
                                           "Could not determine the path to the binaries of the Qt installation, maybe the qmake path is wrong?");
    if (m_mkspecUpToDate && m_mkspecFullPath.isEmpty())
        return QCoreApplication::translate("QtVersion", "The default mkspec symlink is broken.");
    return QString();
}

QString BaseQtVersion::warningReason() const
{
    return QString();
}

Utils::FileName BaseQtVersion::qmakeCommand() const
{
    return m_qmakeCommand;
}

bool BaseQtVersion::toolChainAvailable(const QString &id) const
{
    Q_UNUSED(id)
    if (!isValid())
        return false;
    foreach (const ProjectExplorer::Abi &abi, qtAbis())
        if (!ProjectExplorer::ToolChainManager::instance()->findToolChains(abi).isEmpty())
            return true;
    return false;
}

QList<ProjectExplorer::Abi> BaseQtVersion::qtAbis() const
{
    if (m_qtAbis.isEmpty())
        m_qtAbis = detectQtAbis();
    if (m_qtAbis.isEmpty())
        m_qtAbis.append(ProjectExplorer::Abi()); // add empty ABI by default: This is compatible with all TCs.
    return m_qtAbis;
}

bool BaseQtVersion::equals(BaseQtVersion *other)
{
    if (type() != other->type())
        return false;
    if (uniqueId() != other->uniqueId())
        return false;
    if (displayName() != other->displayName())
        return false;

    return true;
}

int BaseQtVersion::uniqueId() const
{
    return m_id;
}

bool BaseQtVersion::isAutodetected() const
{
    return m_isAutodetected;
}

QString BaseQtVersion::autodetectionSource() const
{
    return m_autodetectionSource;
}

void BaseQtVersion::setAutoDetectionSource(const QString &autodetectionSource)
{
    m_autodetectionSource = autodetectionSource;
}

QString BaseQtVersion::displayName() const
{
    return m_displayName;
}

void BaseQtVersion::setDisplayName(const QString &name)
{
    m_displayName = name;
}

QString BaseQtVersion::toHtml(bool verbose) const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body><table>";
    str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "Name:")
        << "</b></td><td>" << displayName() << "</td></tr>";
    if (!isValid()) {
        str << "<tr><td colspan=2><b>"
            << QCoreApplication::translate("BaseQtVersion", "Invalid Qt version")
            << "</b></td></tr>";
    } else {
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "ABI:")
            << "</b></td>";
        const QList<ProjectExplorer::Abi> abis = qtAbis();
        for (int i = 0; i < abis.size(); ++i) {
            if (i)
                str << "<tr><td></td>";
            str << "<td>" << abis.at(i).toString() << "</td></tr>";
        }
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "Source:")
            << "</b></td><td>" << sourcePath().toUserOutput() << "</td></tr>";
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "mkspec:")
            << "</b></td><td>" << mkspec().toUserOutput() << "</td></tr>";
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "qmake:")
            << "</b></td><td>" << m_qmakeCommand.toUserOutput() << "</td></tr>";
        ensureMkSpecParsed();
        if (!mkspecPath().isEmpty()) {
            if (m_defaultConfigIsDebug || m_defaultConfigIsDebugAndRelease) {
                str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "Default:") << "</b></td><td>"
                    << (m_defaultConfigIsDebug ? "debug" : "release");
                if (m_defaultConfigIsDebugAndRelease)
                    str << " debug_and_release";
                str << "</td></tr>";
            } // default config.
        }
        str << "<tr><td><b>" << QCoreApplication::translate("BaseQtVersion", "Version:")
            << "</b></td><td>" << qtVersionString() << "</td></tr>";
        if (verbose) {
            const QHash<QString,QString> vInfo = versionInfo();
            if (!vInfo.isEmpty()) {
                const QHash<QString,QString>::const_iterator vcend = vInfo.constEnd();
                for (QHash<QString,QString>::const_iterator it = vInfo.constBegin(); it != vcend; ++it)
                    str << "<tr><td><pre>" << it.key() <<  "</pre></td><td>" << it.value() << "</td></tr>";
            }
        }
    }
    str << "</table></body></html>";
    return rc;
}

void BaseQtVersion::updateSourcePath() const
{
    if (!m_sourcePath.isEmpty())
        return;
    updateVersionInfo();
    const QString installData = m_versionInfo.value(QLatin1String("QT_INSTALL_DATA"));
    QString sourcePath = installData;
    QFile qmakeCache(installData + QLatin1String("/.qmake.cache"));
    if (qmakeCache.exists()) {
        qmakeCache.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream stream(&qmakeCache);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (line.startsWith(QLatin1String("QT_SOURCE_TREE"))) {
                sourcePath = line.split(QLatin1Char('=')).at(1).trimmed();
                if (sourcePath.startsWith(QLatin1String("$$quote("))) {
                    sourcePath.remove(0, 8);
                    sourcePath.chop(1);
                }
                break;
            }
        }
    }
    m_sourcePath = Utils::FileName::fromUserInput(sourcePath);
}

Utils::FileName BaseQtVersion::sourcePath() const
{
    updateSourcePath();
    return m_sourcePath;
}

QString BaseQtVersion::designerCommand() const
{
    if (!isValid())
        return QString();
    if (m_designerCommand.isNull())
        m_designerCommand = findQtBinary(Designer);
    return m_designerCommand;
}

QString BaseQtVersion::linguistCommand() const
{
    if (!isValid())
        return QString();
    if (m_linguistCommand.isNull())
        m_linguistCommand = findQtBinary(Linguist);
    return m_linguistCommand;
}

QString BaseQtVersion::qmlviewerCommand() const
{
    if (!isValid())
        return QString();

    if (m_qmlviewerCommand.isNull())
        m_qmlviewerCommand = findQtBinary(QmlViewer);
    return m_qmlviewerCommand;
}

QString BaseQtVersion::findQtBinary(Binaries binary) const
{
    QString baseDir;
    if (qtVersion() < QtVersionNumber(5, 0, 0)) {
        baseDir = versionInfo().value(QLatin1String("QT_INSTALL_BINS"));
    } else {
        ensureMkSpecParsed();
        switch (binary) {
        case QmlViewer:
            baseDir = m_mkspecValues.value(QLatin1String("QT.declarative.bins"));
            break;
        case Designer:
        case Linguist:
            baseDir = m_mkspecValues.value(QLatin1String("QT.designer.bins"));
            break;
        case Uic:
            baseDir = versionInfo().value(QLatin1String("QT_INSTALL_BINS"));
            break;
        default:
            // Can't happen
            Q_ASSERT(false);
        }
    }

    if (baseDir.isEmpty())
        return QString();
    if (!baseDir.endsWith(QLatin1Char('/')))
        baseDir += QLatin1Char('/');

    QStringList possibleCommands;
    switch (binary) {
    case QmlViewer: {
        if (qtVersion() < QtVersionNumber(5, 0, 0)) {
#if defined(Q_OS_WIN)
            possibleCommands << QLatin1String("qmlviewer.exe");
#elif defined(Q_OS_MAC)
            possibleCommands << QLatin1String("QMLViewer.app/Contents/MacOS/QMLViewer");
#else
            possibleCommands << QLatin1String("qmlviewer");
#endif
        } else {
#if defined(Q_OS_WIN)
            possibleCommands << QLatin1String("qmlscene.exe");
#else
            possibleCommands << QLatin1String("qmlscene");
#endif
        }
    }
        break;
    case Designer:
#if defined(Q_OS_WIN)
        possibleCommands << QLatin1String("designer.exe");
#elif defined(Q_OS_MAC)
        possibleCommands << QLatin1String("Designer.app/Contents/MacOS/Designer");
#else
        possibleCommands << QLatin1String("designer");
#endif
        break;
    case Linguist:
#if defined(Q_OS_WIN)
        possibleCommands << QLatin1String("linguist.exe");
#elif defined(Q_OS_MAC)
        possibleCommands << QLatin1String("Linguist.app/Contents/MacOS/Linguist");
#else
        possibleCommands << QLatin1String("linguist");
#endif
        break;
    case Uic:
#ifdef Q_OS_WIN
        possibleCommands << QLatin1String("uic.exe");
#else
        possibleCommands << QLatin1String("uic-qt4") << QLatin1String("uic4") << QLatin1String("uic");
#endif
        break;
    default:
        Q_ASSERT(false);
    }
    foreach (const QString &possibleCommand, possibleCommands) {
        const QString fullPath = baseDir + possibleCommand;
        if (QFileInfo(fullPath).isFile())
            return QDir::cleanPath(fullPath);
    }
    return QString();
}

QString BaseQtVersion::uicCommand() const
{
    if (!isValid())
        return QString();
    if (!m_uicCommand.isNull())
        return m_uicCommand;
    m_uicCommand = findQtBinary(Uic);
    return m_uicCommand;
}

QString BaseQtVersion::systemRoot() const
{
    return QString();
}

void BaseQtVersion::updateMkspec() const
{
    if (uniqueId() == -1 || m_mkspecUpToDate)
        return;

    m_mkspecUpToDate = true;
    m_mkspecFullPath = mkspecFromVersionInfo(versionInfo());

    m_mkspec = m_mkspecFullPath;
    if (m_mkspecFullPath.isEmpty())
        return;

    Utils::FileName baseMkspecDir = Utils::FileName::fromUserInput(versionInfo().value(QLatin1String("QMAKE_MKSPECS")));
    if (baseMkspecDir.isEmpty())
        baseMkspecDir = Utils::FileName::fromUserInput(versionInfo().value(QLatin1String("QT_INSTALL_DATA"))
                                                       + QLatin1String("/mkspecs"));

    if (m_mkspec.isChildOf(baseMkspecDir)) {
        m_mkspec = m_mkspec.relativeChildPath(baseMkspecDir);
//        qDebug() << "Setting mkspec to"<<mkspec;
    } else {
        Utils::FileName sourceMkSpecPath = sourcePath().appendPath(QLatin1String("mkspecs"));
        if (m_mkspec.isChildOf(sourceMkSpecPath)) {
            m_mkspec = m_mkspec.relativeChildPath(sourceMkSpecPath);
        } else {
            // Do nothing
        }
    }
}

void BaseQtVersion::ensureMkSpecParsed() const
{
    if (m_mkspecReadUpToDate)
        return;
    m_mkspecReadUpToDate = true;

    if (mkspecPath().isEmpty())
        return;

    ProFileOption option;
    option.properties = versionInfo();
    ProMessageHandler msgHandler(true);
    ProFileCacheManager::instance()->incRefCount();
    ProFileParser parser(ProFileCacheManager::instance()->cache(), &msgHandler);
    ProFileEvaluator evaluator(&option, &parser, &msgHandler);
    if (ProFile *pro = parser.parsedProFile(mkspecPath().toString() + QLatin1String("/qmake.conf"))) {
        evaluator.setCumulative(false);
        evaluator.accept(pro, ProFileEvaluator::LoadProOnly);
        pro->deref();
    }

    parseMkSpec(&evaluator);

    ProFileCacheManager::instance()->decRefCount();
}

void BaseQtVersion::parseMkSpec(ProFileEvaluator *evaluator) const
{
    QStringList configValues = evaluator->values(QLatin1String("CONFIG"));
    m_defaultConfigIsDebugAndRelease = false;
    foreach (const QString &value, configValues) {
        if (value == QLatin1String("debug"))
            m_defaultConfigIsDebug = true;
        else if (value == QLatin1String("release"))
            m_defaultConfigIsDebug = false;
        else if (value == QLatin1String("build_all"))
            m_defaultConfigIsDebugAndRelease = true;
    }
    const QString designerBins = QLatin1String("QT.designer.bins");
    const QString declarativeBins = QLatin1String("QT.declarative.bins");
    m_mkspecValues.insert(designerBins, evaluator->value(designerBins));
    m_mkspecValues.insert(declarativeBins, evaluator->value(declarativeBins));
}

Utils::FileName BaseQtVersion::mkspec() const
{
    updateMkspec();
    return m_mkspec;
}

Utils::FileName BaseQtVersion::mkspecPath() const
{
    updateMkspec();
    return m_mkspecFullPath;
}

bool BaseQtVersion::hasMkspec(const Utils::FileName &spec) const
{
    updateVersionInfo();
    QFileInfo fi;
    fi.setFile(QDir::fromNativeSeparators(m_versionInfo.value(QLatin1String("QMAKE_MKSPECS")))
               + QLatin1Char('/') + spec.toString());
    if (fi.isDir())
        return true;
    fi.setFile(sourcePath().toString() + QLatin1String("/mkspecs/") + spec.toString());
    return fi.isDir();
}

BaseQtVersion::QmakeBuildConfigs BaseQtVersion::defaultBuildConfig() const
{
    ensureMkSpecParsed();
    BaseQtVersion::QmakeBuildConfigs result = BaseQtVersion::QmakeBuildConfig(0);

    if (m_defaultConfigIsDebugAndRelease)
        result = BaseQtVersion::BuildAll;
    if (m_defaultConfigIsDebug)
        result = result | BaseQtVersion::DebugBuild;
    return result;
}

QString BaseQtVersion::qtVersionString() const
{
    if (!m_qtVersionString.isNull())
        return m_qtVersionString;
    m_qtVersionString.clear();
    if (m_qmakeIsExecutable) {
        const QString qmake = qmakeCommand().toString();
        m_qtVersionString =
            ProjectExplorer::DebuggingHelperLibrary::qtVersionForQMake(qmake, &m_qmakeIsExecutable);
    } else {
        qWarning("Cannot determine the Qt version: %s cannot be run.", qPrintable(qmakeCommand().toString()));
    }
    return m_qtVersionString;
}

QtVersionNumber BaseQtVersion::qtVersion() const
{
    return QtVersionNumber(qtVersionString());
}

void BaseQtVersion::updateVersionInfo() const
{
    if (m_versionInfoUpToDate)
        return;
    if (!m_qmakeIsExecutable) {
        qWarning("Cannot update Qt version information: %s cannot be run.",
                 qPrintable(qmakeCommand().toString()));
        return;
    }

    // extract data from qmake executable
    m_versionInfo.clear();
    m_installed = true;
    m_hasExamples = false;
    m_hasDocumentation = false;
    m_hasDebuggingHelper = false;
    m_hasQmlDump = false;
    m_hasQmlDebuggingLibrary = false;
    m_hasQmlObserver = false;

    if (!queryQMakeVariables(qmakeCommand(), &m_versionInfo, &m_qmakeIsExecutable))
        return;

    const QString installDataKey = QLatin1String("QT_INSTALL_DATA");
    const QString installBinsKey = QLatin1String("QT_INSTALL_BINS");
    const QString installHeadersKey = QLatin1String("QT_INSTALL_HEADERS");
    if (m_versionInfo.contains(installDataKey)) {
        const QString qtInstallData = m_versionInfo.value(installDataKey);
        const QString qtInstallBins = m_versionInfo.value(installBinsKey);
        const QString qtHeaderData = m_versionInfo.value(installHeadersKey);
        m_versionInfo.insert(QLatin1String("QMAKE_MKSPECS"),
                             QDir::cleanPath(qtInstallData + QLatin1String("/mkspecs")));

        if (!qtInstallData.isEmpty()) {
            m_hasDebuggingHelper = !ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData).isEmpty();
            m_hasQmlDump
                    = !QmlDumpTool::toolForQtPaths(qtInstallData, qtInstallBins, qtHeaderData, false).isEmpty()
                    || !QmlDumpTool::toolForQtPaths(qtInstallData, qtInstallBins, qtHeaderData, true).isEmpty();
            m_hasQmlDebuggingLibrary
                    = !QmlDebuggingLibrary::libraryByInstallData(qtInstallData, false).isEmpty()
                || !QmlDebuggingLibrary::libraryByInstallData(qtInstallData, true).isEmpty();
            m_hasQmlObserver = !QmlObserverTool::toolByInstallData(qtInstallData).isEmpty();
        }
    }

    // Now check for a qt that is configured with a prefix but not installed
    if (m_versionInfo.contains(installBinsKey)) {
        QFileInfo fi(m_versionInfo.value(installBinsKey));
        if (!fi.exists())
            m_installed = false;
    }
    if (m_versionInfo.contains(installHeadersKey)) {
        const QFileInfo fi(m_versionInfo.value(installHeadersKey));
        if (!fi.exists())
            m_installed = false;
    }
    const QString installDocsKey = QLatin1String("QT_INSTALL_DOCS");
    if (m_versionInfo.contains(installDocsKey)) {
        const QFileInfo fi(m_versionInfo.value(installDocsKey));
        if (fi.exists())
            m_hasDocumentation = true;
    }
    const QString installExamplesKey = QLatin1String("QT_INSTALL_EXAMPLES");
    if (m_versionInfo.contains(installExamplesKey)) {
        const QFileInfo fi(m_versionInfo.value(installExamplesKey));
        if (fi.exists())
            m_hasExamples = true;
    }
    const QString installDemosKey = QLatin1String("QT_INSTALL_DEMOS");
    if (m_versionInfo.contains(installDemosKey)) {
        const QFileInfo fi(m_versionInfo.value(installDemosKey));
        if (fi.exists())
            m_hasDemos = true;
    }

    m_versionInfoUpToDate = true;
}

QHash<QString,QString> BaseQtVersion::versionInfo() const
{
    updateVersionInfo();
    return m_versionInfo;
}

bool BaseQtVersion::hasDocumentation() const
{
    updateVersionInfo();
    return m_hasDocumentation;
}

QString BaseQtVersion::documentationPath() const
{
    updateVersionInfo();
    return m_versionInfo.value(QLatin1String("QT_INSTALL_DOCS"));
}

bool BaseQtVersion::hasDemos() const
{
    updateVersionInfo();
    return m_hasDemos;
}

QString BaseQtVersion::demosPath() const
{
    updateVersionInfo();
    return m_versionInfo.value(QLatin1String("QT_INSTALL_DEMOS"));
}

QString BaseQtVersion::frameworkInstallPath() const
{
#ifdef Q_OS_MAC
    updateVersionInfo();
    return m_versionInfo.value(QLatin1String("QT_INSTALL_LIBS"));
#else
    return QString();
#endif
}

bool BaseQtVersion::hasExamples() const
{
    updateVersionInfo();
    return m_hasExamples;
}

QString BaseQtVersion::examplesPath() const
{
    updateVersionInfo();
    return m_versionInfo.value(QLatin1String("QT_INSTALL_EXAMPLES"));
}

QList<ProjectExplorer::HeaderPath> BaseQtVersion::systemHeaderPathes() const
{
    QList<ProjectExplorer::HeaderPath> result;
    result.append(ProjectExplorer::HeaderPath(mkspecPath().toString(), ProjectExplorer::HeaderPath::GlobalHeaderPath));
    return result;
}

void BaseQtVersion::addToEnvironment(Utils::Environment &env) const
{
    env.set(QLatin1String("QTDIR"), QDir::toNativeSeparators(versionInfo().value(QLatin1String("QT_INSTALL_DATA"))));
    env.prependOrSetPath(versionInfo().value(QLatin1String("QT_INSTALL_BINS")));
    env.prependOrSetLibrarySearchPath(versionInfo().value(QLatin1String("QT_INSTALL_LIBS")));
}

bool BaseQtVersion::hasGdbDebuggingHelper() const
{
    updateVersionInfo();
    return m_hasDebuggingHelper;
}


bool BaseQtVersion::hasQmlDump() const
{
    updateVersionInfo();
    return m_hasQmlDump;
}

bool BaseQtVersion::needsQmlDump() const
{
    updateVersionInfo();
    return qtVersion() < QtVersionNumber(4, 8, 0);
}

bool BaseQtVersion::hasQmlDebuggingLibrary() const
{
    updateVersionInfo();
    return m_hasQmlDebuggingLibrary;
}

bool BaseQtVersion::needsQmlDebuggingLibrary() const
{
    updateVersionInfo();
    return qtVersion() < QtVersionNumber(4, 8, 0);
}

bool BaseQtVersion::hasQmlObserver() const
{
    updateVersionInfo();
    return m_hasQmlObserver;
}

Utils::Environment BaseQtVersion::qmlToolsEnvironment() const
{
    // FIXME: This seems broken!
    Utils::Environment environment = Utils::Environment::systemEnvironment();
    addToEnvironment(environment);

    // add preferred tool chain, as that is how the tools are built, compare QtVersion::buildDebuggingHelperLibrary
    if (!qtAbis().isEmpty()) {
        QList<ProjectExplorer::ToolChain *> alltc =
                ProjectExplorer::ToolChainManager::instance()->findToolChains(qtAbis().at(0));
        if (!alltc.isEmpty())
            alltc.first()->addToEnvironment(environment);
    }

    return environment;
}

QString BaseQtVersion::gdbDebuggingHelperLibrary() const
{
    QString qtInstallData = versionInfo().value(QLatin1String("QT_INSTALL_DATA"));
    if (qtInstallData.isEmpty())
        return QString();
    return ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryByInstallData(qtInstallData);
}

QString BaseQtVersion::qmlDumpTool(bool debugVersion) const
{
    const QString qtInstallData = versionInfo().value(QLatin1String("QT_INSTALL_DATA"));
    if (qtInstallData.isEmpty())
        return QString();
    const QString qtInstallBins = versionInfo().value(QLatin1String("QT_INSTALL_BINS"));
    const QString qtHeaderData = versionInfo().value(QLatin1String("QT_INSTALL_HEADERS"));
    return QmlDumpTool::toolForQtPaths(qtInstallData, qtInstallBins, qtHeaderData, debugVersion);
}

QString BaseQtVersion::qmlDebuggingHelperLibrary(bool debugVersion) const
{
    QString qtInstallData = versionInfo().value(QLatin1String("QT_INSTALL_DATA"));
    if (qtInstallData.isEmpty())
        return QString();
    return QmlDebuggingLibrary::libraryByInstallData(qtInstallData, debugVersion);
}

QString BaseQtVersion::qmlObserverTool() const
{
    QString qtInstallData = versionInfo().value(QLatin1String("QT_INSTALL_DATA"));
    if (qtInstallData.isEmpty())
        return QString();
    return QmlObserverTool::toolByInstallData(qtInstallData);
}

QStringList BaseQtVersion::debuggingHelperLibraryLocations() const
{
    QString qtInstallData = versionInfo().value(QLatin1String("QT_INSTALL_DATA"));
    if (qtInstallData.isEmpty())
        return QStringList();
    return ProjectExplorer::DebuggingHelperLibrary::debuggingHelperLibraryDirectories(qtInstallData);
}

bool BaseQtVersion::supportsBinaryDebuggingHelper() const
{
    if (!isValid())
        return false;
    return true;
}

void BaseQtVersion::recheckDumper()
{
    m_versionInfoUpToDate = false;
}

bool BaseQtVersion::supportsShadowBuilds() const
{
    return true;
}

QList<ProjectExplorer::Task> BaseQtVersion::reportIssuesImpl(const QString &proFile, const QString &buildDir)
{
    QList<ProjectExplorer::Task> results;

    QString tmpBuildDir = QDir(buildDir).absolutePath();
    if (!tmpBuildDir.endsWith(QLatin1Char('/')))
        tmpBuildDir.append(QLatin1Char('/'));

    if (!isValid()) {
        //: %1: Reason for being invalid
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion", "The Qt version is invalid: %1").arg(invalidReason());
        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Error, msg, QString(), -1,
                                             QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    QFileInfo qmakeInfo = qmakeCommand().toFileInfo();
    if (!qmakeInfo.exists() ||
        !qmakeInfo.isExecutable()) {
        //: %1: Path to qmake executable
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion",
                                                        "The qmake command \"%1\" was not found or is not executable.").arg(qmakeCommand().toUserOutput());
        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Error, msg, QString(), -1,
                                             QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    QString sourcePath = QFileInfo(proFile).absolutePath();
    const QChar slash = QLatin1Char('/');
    if (!sourcePath.endsWith(slash))
        sourcePath.append(slash);
    if ((tmpBuildDir.startsWith(sourcePath)) && (tmpBuildDir != sourcePath)) {
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion",
                                                        "Qmake does not support build directories below the source directory.");
        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Warning, msg, QString(), -1,
                                             QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    } else if (tmpBuildDir.count(slash) != sourcePath.count(slash) && qtVersion() < QtVersionNumber(4,8, 0)) {
        const QString msg = QCoreApplication::translate("Qt4ProjectManager::QtVersion",
                                                        "The build directory needs to be at the same level as the source directory.");

        results.append(ProjectExplorer::Task(ProjectExplorer::Task::Warning, msg, QString(), -1,
                                             QLatin1String(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    }

    return results;
}

QList<ProjectExplorer::Task>
BaseQtVersion::reportIssues(const QString &proFile, const QString &buildDir)
{
    QList<ProjectExplorer::Task> results = reportIssuesImpl(proFile, buildDir);
    qSort(results);
    return results;
}

ProjectExplorer::IOutputParser *BaseQtVersion::createOutputParser() const
{
    return new ProjectExplorer::GnuMakeParser;
}

QtConfigWidget *BaseQtVersion::createConfigurationWidget() const
{
    return 0;
}

bool BaseQtVersion::queryQMakeVariables(const Utils::FileName &binary, QHash<QString, QString> *versionInfo)
{
    bool qmakeIsExecutable;
    return BaseQtVersion::queryQMakeVariables(binary, versionInfo, &qmakeIsExecutable);
}

bool BaseQtVersion::queryQMakeVariables(const Utils::FileName &binary, QHash<QString, QString> *versionInfo,
                                        bool *qmakeIsExecutable)
{
    const int timeOutMS = 30000; // Might be slow on some machines.
    const QFileInfo qmake = binary.toFileInfo();
    *qmakeIsExecutable = qmake.exists() && qmake.isExecutable() && !qmake.isDir();
    if (!*qmakeIsExecutable)
        return false;
    static const char * const variables[] = {
             "QT_VERSION",
             "QT_INSTALL_DATA",
             "QT_INSTALL_LIBS",
             "QT_INSTALL_HEADERS",
             "QT_INSTALL_DEMOS",
             "QT_INSTALL_EXAMPLES",
             "QT_INSTALL_CONFIGURATION",
             "QT_INSTALL_TRANSLATIONS",
             "QT_INSTALL_PLUGINS",
             "QT_INSTALL_BINS",
             "QT_INSTALL_DOCS",
             "QT_INSTALL_PREFIX",
             "QT_INSTALL_IMPORTS",
             "QMAKEFEATURES"
        };
    const QString queryArg = QLatin1String("-query");
    QStringList args;
    for (uint i = 0; i < sizeof variables / sizeof variables[0]; ++i)
        args << queryArg << QLatin1String(variables[i]);
    QProcess process;
    process.start(qmake.absoluteFilePath(), args, QIODevice::ReadOnly);
    if (!process.waitForStarted()) {
        *qmakeIsExecutable = false;
        qWarning("Cannot start '%s': %s", qPrintable(binary.toUserOutput()), qPrintable(process.errorString()));
        return false;
    }
    if (!process.waitForFinished(timeOutMS)) {
        Utils::SynchronousProcess::stopProcess(process);
        qWarning("Timeout running '%s' (%dms).", qPrintable(binary.toUserOutput()), timeOutMS);
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *qmakeIsExecutable = false;
        qWarning("'%s' crashed.", qPrintable(binary.toUserOutput()));
        return false;
    }
    QByteArray output = process.readAllStandardOutput();
    QTextStream stream(&output);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        const int index = line.indexOf(QLatin1Char(':'));
        if (index != -1) {
            const QString value = QDir::fromNativeSeparators(line.mid(index+1));
            if (value != QLatin1String("**Unknown**"))
                versionInfo->insert(line.left(index), value);
        }
    }
    return true;
}

Utils::FileName BaseQtVersion::mkspecFromVersionInfo(const QHash<QString, QString> &versionInfo)
{
    Utils::FileName baseMkspecDir = Utils::FileName::fromUserInput(versionInfo.value(QLatin1String("QMAKE_MKSPECS")));
    if (baseMkspecDir.isEmpty())
        baseMkspecDir = Utils::FileName::fromUserInput(versionInfo.value(QLatin1String("QT_INSTALL_DATA")) + QLatin1String("/mkspecs"));
    if (baseMkspecDir.isEmpty())
        return Utils::FileName();

    Utils::FileName mkspecFullPath = Utils::FileName::fromString(baseMkspecDir.toString() + QLatin1String("/default"));

    // qDebug() << "default mkspec is located at" << mkspecFullPath;

#ifdef Q_OS_WIN
    QFile f2(mkspecFullPath.toString() + "/qmake.conf");
    if (f2.exists() && f2.open(QIODevice::ReadOnly)) {
        while (!f2.atEnd()) {
            QByteArray line = f2.readLine();
            if (line.startsWith("QMAKESPEC_ORIGINAL")) {
                const QList<QByteArray> &temp = line.split('=');
                if (temp.size() == 2) {
                    QString possibleFullPath = temp.at(1).trimmed();
                    // We sometimes get a mix of different slash styles here...
                    possibleFullPath = possibleFullPath.replace('\\', '/');
                    if (QFileInfo(possibleFullPath).exists()) // Only if the path exists
                        mkspecFullPath = Utils::FileName::fromUserInput(possibleFullPath);
                }
                break;
            }
        }
        f2.close();
    }
#elif defined(Q_OS_MAC)
    QFile f2(mkspecFullPath.toString() + "/qmake.conf");
    if (f2.exists() && f2.open(QIODevice::ReadOnly)) {
        while (!f2.atEnd()) {
            QByteArray line = f2.readLine();
            if (line.startsWith("MAKEFILE_GENERATOR")) {
                const QList<QByteArray> &temp = line.split('=');
                if (temp.size() == 2) {
                    const QByteArray &value = temp.at(1);
                    if (value.contains("XCODE")) {
                        // we don't want to generate xcode projects...
//                      qDebug() << "default mkspec is xcode, falling back to g++";
                        mkspecFullPath = baseMkspecDir.appendPath("macx-g++");
                    }
                    //resolve mkspec link
                    mkspecFullPath = Utils::FileName::fromString(mkspecFullPath.toFileInfo().canonicalFilePath());
                }
                break;
            }
        }
        f2.close();
    }
#else
    mkspecFullPath = Utils::FileName::fromString(mkspecFullPath.toFileInfo().canonicalFilePath());
#endif

    return mkspecFullPath;
}

QString BaseQtVersion::qtCorePath(const QHash<QString,QString> &versionInfo, const QString &versionString)
{
    QStringList dirs;
    dirs << versionInfo.value(QLatin1String("QT_INSTALL_LIBS"))
         << versionInfo.value(QLatin1String("QT_INSTALL_BINS"));

    QFileInfoList staticLibs;
    foreach (const QString &dir, dirs) {
        if (dir.isEmpty())
            continue;
        QDir d(dir);
        QFileInfoList infoList = d.entryInfoList();
        foreach (const QFileInfo &info, infoList) {
            const QString file = info.fileName();
            if (info.isDir()
                    && file.startsWith(QLatin1String("QtCore"))
                    && file.endsWith(QLatin1String(".framework"))) {
                // handle Framework
                const QString libName = file.left(file.lastIndexOf(QLatin1Char('.')));
                return info.absoluteFilePath() + QLatin1Char('/') + libName;
            }
            if (info.isReadable()) {
                if (file.startsWith(QLatin1String("libQtCore"))
                        || file.startsWith(QLatin1String("QtCore"))) {
                    // Only handle static libs if we can not find dynamic ones:
                    if (file.endsWith(QLatin1String(".a")) || file.endsWith(QLatin1String(".lib")))
                        staticLibs.append(info);
                    else if (file.endsWith(QLatin1String(".dll"))
                             || file.endsWith(QString::fromLatin1(".so.") + versionString)
                             || file.endsWith(QString::fromLatin1(".so"))
                             || file.endsWith(QLatin1Char('.') + versionString + QLatin1String(".dylib")))
                        return info.absoluteFilePath();
                }
            }
        }
    }
    // Return path to first static library found:
    if (!staticLibs.isEmpty())
        return staticLibs.at(0).absoluteFilePath();
    return QString();
}

QList<ProjectExplorer::Abi> BaseQtVersion::qtAbisFromLibrary(const QString &coreLibrary)
{
    return ProjectExplorer::Abi::abisOfBinary(coreLibrary);
}
