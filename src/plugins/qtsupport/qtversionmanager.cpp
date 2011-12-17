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

#include "qtversionmanager.h"

#include "qtversionfactory.h"

#include <projectexplorer/debugginghelper.h>
// only for legay restore
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/gcctoolchain.h>


#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/persistentsettings.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#ifdef Q_OS_WIN
#    include <utils/winutils.h>
#endif

#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtGui/QMainWindow>

#include <algorithm>

using namespace QtSupport;
using namespace QtSupport::Internal;

using ProjectExplorer::DebuggingHelperLibrary;

static const char QTVERSION_DATA_KEY[] = "QtVersion.";
static const char QTVERSION_TYPE_KEY[] = "QtVersion.Type";
static const char QTVERSION_COUNT_KEY[] = "QtVersion.Count";
static const char OLDQTVERSION_COUNT_KEY[] = "QtVersion.Old.Count";
static const char OLDQTVERSION_DATA_KEY[] = "QtVersion.Old.";
static const char OLDQTVERSION_SDKSOURCE[] = "QtVersion.Old.SdkSource";
static const char OLDQTVERSION_PATH[] = "QtVersion.Old.Path";
static const char QTVERSION_FILE_VERSION_KEY[] = "Version";
static const char QTVERSION_FILENAME[] = "/qtversion.xml";

// legacy settings
static const char QtVersionsSectionName[] = "QtVersions";

enum { debug = 0 };

template<class T>
static T *createToolChain(const QString &id)
{
    QList<ProjectExplorer::ToolChainFactory *> factories =
            ExtensionSystem::PluginManager::instance()->getObjects<ProjectExplorer::ToolChainFactory>();
    foreach (ProjectExplorer::ToolChainFactory *f, factories) {
       if (f->id() == id) {
           Q_ASSERT(f->canCreate());
           return static_cast<T *>(f->create());
       }
    }
    return 0;
}

static QString settingsFileName()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QFileInfo settingsLocation(pm->settings()->fileName());
    return settingsLocation.absolutePath() + QLatin1String(QTVERSION_FILENAME);
}


// prefer newer qts otherwise compare on id
bool qtVersionNumberCompare(BaseQtVersion *a, BaseQtVersion *b)
{
    return a->qtVersion() > b->qtVersion() || (a->qtVersion() == b->qtVersion() && a->uniqueId() < b->uniqueId());
}

// --------------------------------------------------------------------------
// QtVersionManager
// --------------------------------------------------------------------------
QtVersionManager *QtVersionManager::m_self = 0;

QtVersionManager::QtVersionManager()
{
    m_self = this;
    m_idcount = 1;

    qRegisterMetaType<Utils::FileName>();
}

void QtVersionManager::extensionsInitialized()
{
    bool success = restoreQtVersions();
    if (!success)
        success = legacyRestore();
    updateFromInstaller();
    if (!success) {
        // We did neither restore our settings or upgraded
        // in that case figure out if there's a qt in path
        // and add it to the qt versions
        findSystemQt();
    }

    updateSettings();
    saveQtVersions();
}

QtVersionManager::~QtVersionManager()
{
    qDeleteAll(m_versions);
    m_versions.clear();
}

QtVersionManager *QtVersionManager::instance()
{
    return m_self;
}

bool QtVersionManager::restoreQtVersions()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QList<QtVersionFactory *> factories = pm->getObjects<QtVersionFactory>();

    Utils::PersistentSettingsReader reader;
    if (!reader.load(settingsFileName()))
        return false;
    QVariantMap data = reader.restoreValues();

    // Check version:
    int version = data.value(QLatin1String(QTVERSION_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return false;


    int count = data.value(QLatin1String(QTVERSION_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(QTVERSION_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        const QVariantMap qtversionMap = data.value(key).toMap();
        const QString type = qtversionMap.value(QTVERSION_TYPE_KEY).toString();

        bool restored = false;
        foreach (QtVersionFactory *f, factories) {
            if (f->canRestore(type)) {
                if (BaseQtVersion *qtv = f->restore(type, qtversionMap)) {
                    if (m_versions.contains(qtv->uniqueId())) {
                        // This shouldn't happen, we are restoring the same id multiple times?
                        qWarning() << "A Qt version with id"<<qtv->uniqueId()<<"already exists";
                        delete qtv;
                    } else {
                        m_versions.insert(qtv->uniqueId(), qtv);
                        m_idcount = qtv->uniqueId() > m_idcount ? qtv->uniqueId() : m_idcount;
                        restored = true;
                        break;
                    }
                }
            }
        }
        if (!restored)
            qWarning("Warning: Unable to restore Qt version '%s' stored in %s.",
                     qPrintable(type),
                     qPrintable(QDir::toNativeSeparators(settingsFileName())));
    }
    ++m_idcount;
    return true;
}

void QtVersionManager::updateFromInstaller()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    QList<QtVersionFactory *> factories = pm->getObjects<QtVersionFactory>();
    Utils::PersistentSettingsReader reader;
    if (!reader.load(QFileInfo(pm->globalSettings()->fileName()).absolutePath()
                     + QLatin1String(QTVERSION_FILENAME)))
        return;

    QVariantMap data = reader.restoreValues();

    if (debug) {
        qDebug()<< "======= Existing Qt versions =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }
    }

    int oldcount = data.value(QLatin1String(OLDQTVERSION_COUNT_KEY), 0).toInt();
    for (int i=0; i < oldcount; ++i) {
        const QString key = QString::fromLatin1(OLDQTVERSION_DATA_KEY) +QString::number(i);
        if (!data.contains(key))
            break;
        QVariantMap map = data.value(key).toMap();
        Utils::FileName path = Utils::FileName::fromString(map.value(OLDQTVERSION_PATH).toString());
        QString autodetectionSource = map.value(OLDQTVERSION_SDKSOURCE).toString();
        foreach (BaseQtVersion *v, m_versions) {
            if (v->qmakeCommand() == path) {
                if (v->autodetectionSource().isEmpty()) {
                    v->setAutoDetectionSource(autodetectionSource);
                } else {
                    if (debug)
                        qDebug() << "## Conflicting autodetictonSource for"<<path.toString()<<"\n"
                                 <<"     version retains"<<v->autodetectionSource();
                }
                // No break, we want to mark all qt versions matching that path
                // There's no way for us to decide whether this qt was added
                // by the user or by the installer, so we treat them all as coming
                // from the installer. Thus removing/updating them deletes/updates them all
                // Note: This only applies to versions that are marked via QtVersion.Old
            }
        }
    }

    if (debug) {
        qDebug()<< "======= After using OLD QtVersion data to mark versions =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }

        qDebug()<< "======= Adding sdk versions =======";
    }
    QStringList sdkVersions;
    int count = data.value(QLatin1String(QTVERSION_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(QTVERSION_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;

        QVariantMap qtversionMap = data.value(key).toMap();
        const QString type = qtversionMap.value(QTVERSION_TYPE_KEY).toString();
        const QString autoDetectionSource = qtversionMap.value(QLatin1String("autodetectionSource")).toString();
        sdkVersions << autoDetectionSource;
        int id = -1; // see BaseQtVersion::fromMap()
        QtVersionFactory *factory = 0;
        foreach (QtVersionFactory *f, factories) {
            if (f->canRestore(type)) {
                factory = f;
            }
        }
        if (!factory) {
            if (debug)
                qDebug("Warning: Unable to find factory for type '%s'", qPrintable(type));
            continue;
        }
        // First try to find a existing qt version to update
        bool restored = false;
        foreach (BaseQtVersion *v, m_versions) {
            if (v->autodetectionSource() == autoDetectionSource) {
                id = v->uniqueId();
                if (debug)
                    qDebug() << " Qt version found with same autodetection source" << autoDetectionSource << " => Migrating id:" << id;
                removeVersion(v);
                qtversionMap[QLatin1String("Id")] = id;

                if (BaseQtVersion *qtv = factory->restore(type, qtversionMap)) {
                    Q_ASSERT(qtv->isAutodetected());
                    addVersion(qtv);
                    restored = true;
                }
            }
        }
        // Create a new qtversion
        if (!restored) { // didn't replace any existing versions
            if (debug)
                qDebug() << " No Qt version found matching" << autoDetectionSource << " => Creating new version";
            if (BaseQtVersion *qtv = factory->restore(type, qtversionMap)) {
                Q_ASSERT(qtv->isAutodetected());
                addVersion(qtv);
                restored = true;
            }
        }
        if (!restored)
            if (debug)
                qDebug("Warning: Unable to update qtversion '%s' from sdk installer.",
                       qPrintable(autoDetectionSource));
    }

    if (debug) {
        qDebug() << "======= Before removing outdated sdk versions =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }
    }
    foreach (BaseQtVersion *qtVersion, QtVersionManager::instance()->versions()) {
        if (qtVersion->autodetectionSource().startsWith("SDK.")) {
            if (!sdkVersions.contains(qtVersion->autodetectionSource())) {
                if (debug)
                    qDebug() << "  removing version"<<qtVersion->autodetectionSource();
                removeVersion(qtVersion);
            }
        }
    }

    if (debug) {
        qDebug()<< "======= End result =======";
        foreach (BaseQtVersion *version, m_versions) {
            qDebug() << version->qmakeCommand().toString() << "id:"<<version->uniqueId();
            qDebug() << "  autodetection source:"<< version->autodetectionSource();
            qDebug() << "";
        }
    }
}

void QtVersionManager::saveQtVersions()
{
    Utils::PersistentSettingsWriter writer;
    writer.saveValue(QLatin1String(QTVERSION_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (BaseQtVersion *qtv, m_versions) {
        QVariantMap tmp = qtv->toMap();
        if (tmp.isEmpty())
            continue;
        tmp.insert(QTVERSION_TYPE_KEY, qtv->type());
        writer.saveValue(QString::fromLatin1(QTVERSION_DATA_KEY) + QString::number(count), tmp);
        ++count;

    }
    writer.saveValue(QLatin1String(QTVERSION_COUNT_KEY), count);
    writer.save(settingsFileName(), "QtCreatorQtVersions", Core::ICore::instance()->mainWindow());
}

void QtVersionManager::findSystemQt()
{
    Utils::FileName systemQMakePath = ProjectExplorer::DebuggingHelperLibrary::findSystemQt(Utils::Environment::systemEnvironment());
    if (systemQMakePath.isNull())
        return;

    BaseQtVersion *version = QtVersionFactory::createQtVersionFromQMakePath(systemQMakePath);
    version->setDisplayName(BaseQtVersion::defaultDisplayName(version->qtVersionString(), systemQMakePath, true));
    m_versions.insert(version->uniqueId(), version);
}

bool QtVersionManager::legacyRestore()
{
    QSettings *s = Core::ICore::instance()->settings();
    if (!s->contains(QLatin1String(QtVersionsSectionName) + QLatin1String("/size")))
        return false;
    int size = s->beginReadArray(QtVersionsSectionName);
    for (int i = 0; i < size; ++i) {
        s->setArrayIndex(i);
        // Find the right id
        // Either something saved or something generated
        // Note: This code assumes that either all ids are read from the settings
        // or generated on the fly.
        int id = s->value("Id", -1).toInt();
        if (id == -1)
            id = getUniqueId();
        else if (m_idcount < id)
            m_idcount = id + 1;

        Utils::FileName qmakePath = Utils::FileName::fromString(s->value("QMakePath").toString());
        if (qmakePath.isEmpty())
            continue; //skip this version

        BaseQtVersion *version = QtVersionFactory::createQtVersionFromLegacySettings(qmakePath, id, s);
        if (!version) // Likely to be a invalid version
            continue;

        if (m_versions.contains(version->uniqueId())) {
            // oh uh;
            delete version;
        } else {
            m_versions.insert(version->uniqueId(), version);
        }
        // Update from 2.1 or earlier:
        QString mingwDir = s->value(QLatin1String("MingwDirectory")).toString();
        if (!mingwDir.isEmpty()) {
            QFileInfo fi(mingwDir + QLatin1String("/bin/g++.exe"));
            if (fi.exists() && fi.isExecutable()) {
                ProjectExplorer::MingwToolChain *tc = createToolChain<ProjectExplorer::MingwToolChain>(ProjectExplorer::Constants::MINGW_TOOLCHAIN_ID);
                if (tc) {
                    tc->setCompilerPath(fi.absoluteFilePath());
                    tc->setDisplayName(tr("MinGW from %1").arg(version->displayName()));
                    // The debugger is set later in the autoDetect method of the MinGw tool chain factory
                    // as the default debuggers are not yet registered.
                    ProjectExplorer::ToolChainManager::instance()->registerToolChain(tc);
                }
            }
        }
        const QString mwcDir = s->value(QLatin1String("MwcDirectory")).toString();
        if (!mwcDir.isEmpty())
            m_pendingMwcUpdates.append(mwcDir);
        const QString gcceDir = s->value(QLatin1String("GcceDirectory")).toString();
        if (!gcceDir.isEmpty())
            m_pendingGcceUpdates.append(gcceDir);

    }
    s->endArray();
    s->remove(QtVersionsSectionName);
    return true;
}

void QtVersionManager::addVersion(BaseQtVersion *version)
{
    QTC_ASSERT(version != 0, return);
    if (m_versions.contains(version->uniqueId()))
        return;

    int uniqueId = version->uniqueId();
    m_versions.insert(uniqueId, version);

    emit qtVersionsChanged(QList<int>() << uniqueId);
    saveQtVersions();
}

void QtVersionManager::removeVersion(BaseQtVersion *version)
{
    QTC_ASSERT(version != 0, return);
    m_versions.remove(version->uniqueId());
    emit qtVersionsChanged(QList<int>() << version->uniqueId());
    saveQtVersions();
    delete version;
}

bool QtVersionManager::supportsTargetId(const QString &id) const
{
    QList<BaseQtVersion *> versions = QtVersionManager::instance()->versionsForTargetId(id);
    foreach (BaseQtVersion *v, versions)
        if (v->isValid() && v->toolChainAvailable(id))
            return true;
    return false;
}

QList<BaseQtVersion *> QtVersionManager::versionsForTargetId(const QString &id,
                                                             const QtVersionNumber &minimumQtVersion,
                                                             const QtVersionNumber &maximumQtVersion) const
{
    QList<BaseQtVersion *> targetVersions;
    foreach (BaseQtVersion *version, m_versions) {
        if (version->supportsTargetId(id) && version->qtVersion() >= minimumQtVersion
                && version->qtVersion() <= maximumQtVersion)
            targetVersions.append(version);
    }
    qSort(targetVersions.begin(), targetVersions.end(), &qtVersionNumberCompare);
    return targetVersions;
}

QSet<QString> QtVersionManager::supportedTargetIds() const
{
    QSet<QString> results;
    foreach (BaseQtVersion *version, m_versions)
        results.unite(version->supportedTargetIds());
    return results;
}

void QtVersionManager::updateDocumentation()
{
    Core::HelpManager *helpManager = Core::HelpManager::instance();
    Q_ASSERT(helpManager);
    QStringList files;
    foreach (BaseQtVersion *v, m_versions) {
        const QString docPath = v->documentationPath() + QLatin1String("/qch/");
        const QDir versionHelpDir(docPath);
        foreach (const QString &helpFile,
                versionHelpDir.entryList(QStringList() << QLatin1String("*.qch"), QDir::Files))
            files << docPath + helpFile;

    }
    helpManager->registerDocumentation(files);
}

void QtVersionManager::updateDumpFor(const Utils::FileName &qmakeCommand)
{
    foreach (BaseQtVersion *v, versions()) {
        if (v->qmakeCommand() == qmakeCommand)
            v->recheckDumper();
    }
    emit dumpUpdatedFor(qmakeCommand);
}

void QtVersionManager::updateSettings()
{
    updateDocumentation();

    BaseQtVersion *version = 0;
    QList<BaseQtVersion *> candidates;

    // try to find a version which has both, demos and examples
    foreach (BaseQtVersion *version, m_versions) {
        if (version && version->hasExamples() && version->hasDemos())
            candidates.append(version);
    }

    // in SDKs, we want to prefer the Qt version shipping with the SDK
    QSettings *settings = Core::ICore::instance()->settings();
    Utils::FileName preferred = Utils::FileName::fromUserInput(settings->value(QLatin1String("PreferredQMakePath")).toString());
    if (!preferred.isEmpty()) {
#ifdef Q_OS_WIN
        if (!preferred.endsWith(".exe"))
            preferred.append(".exe");
#endif
        foreach (version, candidates) {
            if (version->qmakeCommand() == preferred) {
                emit updateExamples(version->examplesPath(), version->demosPath(), version->sourcePath().toString());
                return;
            }
        }
    }

    // prefer versions with declarative examples
    foreach (version, candidates) {
        if (QDir(version->examplesPath()+"/declarative").exists()) {
            emit updateExamples(version->examplesPath(), version->demosPath(), version->sourcePath().toString());
            return;
        }
    }

    if (!candidates.isEmpty()) {
        version = candidates.first();
        emit updateExamples(version->examplesPath(), version->demosPath(), version->sourcePath().toString());
        return;
    }
    return;

}

int QtVersionManager::getUniqueId()
{
    return m_idcount++;
}

QList<BaseQtVersion *> QtVersionManager::versions() const
{
    QList<BaseQtVersion *> versions;
    foreach (BaseQtVersion *version, m_versions)
        versions << version;
    qSort(versions.begin(), versions.end(), &qtVersionNumberCompare);
    return versions;
}

QList<BaseQtVersion *> QtVersionManager::validVersions() const
{
    QList<BaseQtVersion *> results;
    foreach (BaseQtVersion *v, m_versions) {
        if (v->isValid())
            results.append(v);
    }
    qSort(results.begin(), results.end(), &qtVersionNumberCompare);
    return results;
}

bool QtVersionManager::isValidId(int id) const
{
    return m_versions.contains(id);
}

QString QtVersionManager::popPendingMwcUpdate()
{
    if (m_pendingMwcUpdates.isEmpty())
        return QString();
    return m_pendingMwcUpdates.takeFirst();
}

QString QtVersionManager::popPendingGcceUpdate()
{
    if (m_pendingGcceUpdates.isEmpty())
        return QString();
    return m_pendingGcceUpdates.takeFirst();
}

BaseQtVersion *QtVersionManager::version(int id) const
{
    QMap<int, BaseQtVersion *>::const_iterator it = m_versions.find(id);
    if (it == m_versions.constEnd())
        return 0;
    return it.value();
}

class SortByUniqueId
{
public:
    bool operator()(BaseQtVersion *a, BaseQtVersion *b)
    {
        return a->uniqueId() < b->uniqueId();
    }
};

bool QtVersionManager::equals(BaseQtVersion *a, BaseQtVersion *b)
{
    return a->equals(b);
}

void QtVersionManager::setNewQtVersions(QList<BaseQtVersion *> newVersions)
{
    // We want to preserve the same order as in the settings dialog
    // so we sort a copy
    QList<BaseQtVersion *> sortedNewVersions = newVersions;
    SortByUniqueId sortByUniqueId;
    qSort(sortedNewVersions.begin(), sortedNewVersions.end(), sortByUniqueId);

    QList<int> changedVersions;
    // So we trying to find the minimal set of changed versions,
    // iterate over both sorted list

    // newVersions and oldVersions iterator
    QList<BaseQtVersion *>::const_iterator nit, nend;
    QMap<int, BaseQtVersion *>::const_iterator oit, oend;
    nit = sortedNewVersions.constBegin();
    nend = sortedNewVersions.constEnd();
    oit = m_versions.constBegin();
    oend = m_versions.constEnd();

    while (nit != nend && oit != oend) {
        int nid = (*nit)->uniqueId();
        int oid = (*oit)->uniqueId();
        if (nid < oid) {
            changedVersions.push_back(nid);
            ++nit;
        } else if (oid < nid) {
            changedVersions.push_back(oid);
            ++oit;
        } else {
            if (!equals(*oit, *nit))
                changedVersions.push_back(oid);
            ++oit;
            ++nit;
        }
    }

    while (nit != nend) {
        changedVersions.push_back((*nit)->uniqueId());
        ++nit;
    }

    while (oit != oend) {
        changedVersions.push_back((*oit)->uniqueId());
        ++oit;
    }

    qDeleteAll(m_versions);
    m_versions.clear();
    foreach (BaseQtVersion *v, sortedNewVersions)
        m_versions.insert(v->uniqueId(), v);

    if (!changedVersions.isEmpty())
        updateDocumentation();

    updateSettings();
    saveQtVersions();

    if (!changedVersions.isEmpty())
        emit qtVersionsChanged(changedVersions);
}

// Returns the version that was used to build the project in that directory
// That is returns the directory
// To find out whether we already have a qtversion for that directory call
// QtVersion *QtVersionManager::qtVersionForDirectory(const QString directory);
Utils::FileName QtVersionManager::findQMakeBinaryFromMakefile(const QString &makefile)
{
    bool debugAdding = false;
    QFile fi(makefile);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        QTextStream ts(&fi);
        QRegExp r1("QMAKE\\s*=(.*)");
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (r1.exactMatch(line)) {
                if (debugAdding)
                    qDebug()<<"#~~ QMAKE is:"<<r1.cap(1).trimmed();
                QFileInfo qmake(r1.cap(1).trimmed());
                QString qmakePath = qmake.filePath();
#ifdef Q_OS_WIN
                if (!qmakePath.endsWith(QLatin1String(".exe")))
                    qmakePath.append(QLatin1String(".exe"));
#endif
                // Is qmake still installed?
                QFileInfo fi(qmakePath);
                if (fi.exists()) {
                    return Utils::FileName(fi);
                }
            }
        }
    }
    return Utils::FileName();
}

BaseQtVersion *QtVersionManager::qtVersionForQMakeBinary(const Utils::FileName &qmakePath)
{
   foreach (BaseQtVersion *version, versions()) {
       if (version->qmakeCommand() == qmakePath) {
           return version;
           break;
       }
   }
   return 0;
}

void dumpQMakeAssignments(const QList<QMakeAssignment> &list)
{
    foreach(const QMakeAssignment &qa, list) {
        qDebug()<<qa.variable<<qa.op<<qa.value;
    }
}

QtVersionManager::MakefileCompatible QtVersionManager::makefileIsFor(const QString &makefile, const QString &proFile)
{
    if (proFile.isEmpty())
        return CouldNotParse;

    // The Makefile.Debug / Makefile.Release lack a # Command: line
    if (findQMakeLine(makefile, QLatin1String("# Command:")).trimmed().isEmpty())
        return CouldNotParse;

    QString line = findQMakeLine(makefile, QLatin1String("# Project:")).trimmed();
    if (line.isEmpty())
        return CouldNotParse;

    line = line.mid(line.indexOf(QChar(':')) + 1);
    line = line.trimmed();

    QFileInfo srcFileInfo(QFileInfo(makefile).absoluteDir(), line);
    QFileInfo proFileInfo(proFile);
    return (srcFileInfo == proFileInfo) ? SameProject : DifferentProject;
}

QPair<BaseQtVersion::QmakeBuildConfigs, QString> QtVersionManager::scanMakeFile(const QString &makefile, BaseQtVersion::QmakeBuildConfigs defaultBuildConfig)
{
    if (debug)
        qDebug()<<"ScanMakeFile, the gory details:";
    BaseQtVersion::QmakeBuildConfigs result = defaultBuildConfig;
    QString result2;

    QString line = findQMakeLine(makefile, QLatin1String("# Command:"));
    if (!line.isEmpty()) {
        if (debug)
            qDebug()<<"Found line"<<line;
        line = trimLine(line);
        QList<QMakeAssignment> assignments;
        QList<QMakeAssignment> afterAssignments;
        parseArgs(line, &assignments, &afterAssignments, &result2);

        if (debug) {
            dumpQMakeAssignments(assignments);
            if (!afterAssignments.isEmpty())
                qDebug()<<"-after";
            dumpQMakeAssignments(afterAssignments);
        }

        // Search in assignments for CONFIG(+=,-=,=)(debug,release,debug_and_release)
        // Also remove them from the list
        result = qmakeBuildConfigFromCmdArgs(&assignments, defaultBuildConfig);

        if (debug)
            dumpQMakeAssignments(assignments);

        foreach(const QMakeAssignment &qa, assignments)
            Utils::QtcProcess::addArg(&result2, qa.variable + qa.op + qa.value);
        if (!afterAssignments.isEmpty()) {
            Utils::QtcProcess::addArg(&result2, QLatin1String("-after"));
            foreach(const QMakeAssignment &qa, afterAssignments)
                Utils::QtcProcess::addArg(&result2, qa.variable + qa.op + qa.value);
        }
    }

    // Dump the gathered information:
    if (debug) {
        qDebug()<<"\n\nDumping information from scanMakeFile";
        qDebug()<<"QMake CONFIG variable parsing";
        qDebug()<<"  "<< (result & BaseQtVersion::NoBuild ? "No Build" : QString::number(int(result)));
        qDebug()<<"  "<< (result & BaseQtVersion::DebugBuild ? "debug" : "release");
        qDebug()<<"  "<< (result & BaseQtVersion::BuildAll ? "debug_and_release" : "no debug_and_release");
        qDebug()<<"\nAddtional Arguments";
        qDebug()<<result2;
        qDebug()<<"\n\n";
    }
    return qMakePair(result, result2);
}

QString QtVersionManager::findQMakeLine(const QString &makefile, const QString &key)
{
    QFile fi(makefile);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        QTextStream ts(&fi);
        while (!ts.atEnd()) {
            const QString line = ts.readLine();
            if (line.startsWith(key))
                return line;
        }
    }
    return QString();
}

/// This function trims the "#Command /path/to/qmake" from the the line
QString QtVersionManager::trimLine(const QString line)
{

    // Actually the first space after #Command: /path/to/qmake
    const int firstSpace = line.indexOf(QLatin1Char(' '), 11);
    return line.mid(firstSpace).trimmed();
}

void QtVersionManager::parseArgs(const QString &args, QList<QMakeAssignment> *assignments, QList<QMakeAssignment> *afterAssignments, QString *additionalArguments)
{
    QRegExp regExp("([^\\s\\+-]*)\\s*(\\+=|=|-=|~=)(.*)");
    bool after = false;
    bool ignoreNext = false;
    *additionalArguments = args;
    Utils::QtcProcess::ArgIterator ait(additionalArguments);
    while (ait.next()) {
        if (ignoreNext) {
            // Ignoring
            ignoreNext = false;
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-after")) {
            after = true;
            ait.deleteArg();
        } else if (ait.value().contains(QLatin1Char('='))) {
            if (regExp.exactMatch(ait.value())) {
                QMakeAssignment qa;
                qa.variable = regExp.cap(1);
                qa.op = regExp.cap(2);
                qa.value = regExp.cap(3).trimmed();
                if (after)
                    afterAssignments->append(qa);
                else
                    assignments->append(qa);
            } else {
                qDebug()<<"regexp did not match";
            }
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-o")) {
            ignoreNext = true;
            ait.deleteArg();
#if defined(Q_OS_WIN32)
        } else if (ait.value() == QLatin1String("-win32")) {
#elif defined(Q_OS_MAC)
        } else if (ait.value() == QLatin1String("-macx")) {
#elif defined(Q_OS_QNX6)
        } else if (ait.value() == QLatin1String("-qnx6")) {
#else
        } else if (ait.value() == QLatin1String("-unix")) {
#endif
            ait.deleteArg();
        }
    }
    ait.deleteArg();  // The .pro file is always the last arg
}

/// This function extracts all the CONFIG+=debug, CONFIG+=release
BaseQtVersion::QmakeBuildConfigs QtVersionManager::qmakeBuildConfigFromCmdArgs(QList<QMakeAssignment> *assignments, BaseQtVersion::QmakeBuildConfigs defaultBuildConfig)
{
    BaseQtVersion::QmakeBuildConfigs result = defaultBuildConfig;
    QList<QMakeAssignment> oldAssignments = *assignments;
    assignments->clear();
    foreach(const QMakeAssignment &qa, oldAssignments) {
        if (qa.variable == "CONFIG") {
            QStringList values = qa.value.split(' ');
            QStringList newValues;
            foreach(const QString &value, values) {
                if (value == "debug") {
                    if (qa.op == "+=")
                        result = result  | BaseQtVersion::DebugBuild;
                    else
                        result = result  & ~BaseQtVersion::DebugBuild;
                } else if (value == "release") {
                    if (qa.op == "+=")
                        result = result & ~BaseQtVersion::DebugBuild;
                    else
                        result = result | BaseQtVersion::DebugBuild;
                } else if (value == "debug_and_release") {
                    if (qa.op == "+=")
                        result = result | BaseQtVersion::BuildAll;
                    else
                        result = result & ~BaseQtVersion::BuildAll;
                } else {
                    newValues.append(value);
                }
                QMakeAssignment newQA = qa;
                newQA.value = newValues.join(" ");
                if (!newValues.isEmpty())
                    assignments->append(newQA);
            }
        } else {
            assignments->append(qa);
        }
    }
    return result;
}
