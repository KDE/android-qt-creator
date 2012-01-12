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

#include "cpptoolsplugin.h"
#include "completionsettingspage.h"
#include "cppfilesettingspage.h"
#include "cppcodestylesettingspage.h"
#include "cppclassesfilter.h"
#include "cppfunctionsfilter.h"
#include "cppcurrentdocumentfilter.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "cpplocatorfilter.h"
#include "symbolsfindfilter.h"
#include "cppcompletionassist.h"
#include "cpptoolssettings.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/filemanager.h>
#include <cppeditor/cppeditorconstants.h>

#include <QtCore/QtConcurrentRun>
#include <QtCore/QFutureSynchronizer>
#include <qtconcurrent/runextensions.h>

#include <find/ifindfilter.h>
#include <find/searchresultwindow.h>
#include <utils/filesearch.h>

#include <QtCore/QtPlugin>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QMenu>
#include <QtGui/QAction>

#include <sstream>

using namespace CppTools::Internal;
using namespace CPlusPlus;

enum { debug = 0 };

static CppToolsPlugin *m_instance = 0;

CppToolsPlugin::CppToolsPlugin() :
    m_modelManager(0),
    m_fileSettings(new CppFileSettings)
{
    m_instance = this;
}

CppToolsPlugin::~CppToolsPlugin()
{
    m_instance = 0;
    m_modelManager = 0; // deleted automatically
}

bool CppToolsPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)

    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    m_settings = new CppToolsSettings(this); // force registration of cpp tools settings

    // Objects
    m_modelManager = new CppModelManager(this);
    Core::VcsManager *vcsManager = core->vcsManager();
    Core::FileManager *fileManager = core->fileManager();
    connect(vcsManager, SIGNAL(repositoryChanged(QString)),
            m_modelManager, SLOT(updateModifiedSourceFiles()));
    connect(fileManager, SIGNAL(filesChangedInternally(QStringList)),
            m_modelManager, SLOT(updateSourceFiles(QStringList)));
    addAutoReleasedObject(m_modelManager);

    addAutoReleasedObject(new CppCompletionAssistProvider);
    addAutoReleasedObject(new CppLocatorFilter(m_modelManager));
    addAutoReleasedObject(new CppClassesFilter(m_modelManager));
    addAutoReleasedObject(new CppFunctionsFilter(m_modelManager));
    addAutoReleasedObject(new CppCurrentDocumentFilter(m_modelManager, core->editorManager()));
    addAutoReleasedObject(new CppFileSettingsPage(m_fileSettings));
    addAutoReleasedObject(new SymbolsFindFilter(m_modelManager));
    addAutoReleasedObject(new CppCodeStyleSettingsPage);

    // Menus
    Core::ActionContainer *mtools = am->actionContainer(Core::Constants::M_TOOLS);
    Core::ActionContainer *mcpptools = am->createMenu(CppTools::Constants::M_TOOLS_CPP);
    QMenu *menu = mcpptools->menu();
    menu->setTitle(tr("&C++"));
    menu->setEnabled(true);
    mtools->addMenu(mcpptools);

    // Actions
    Core::Context context(CppEditor::Constants::C_CPPEDITOR);

    QAction *switchAction = new QAction(tr("Switch Header/Source"), this);
    Core::Command *command = am->registerAction(switchAction, Constants::SWITCH_HEADER_SOURCE, context, true);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F4));
    mcpptools->addAction(command);
    connect(switchAction, SIGNAL(triggered()), this, SLOT(switchHeaderSource()));

    return true;
}

void CppToolsPlugin::extensionsInitialized()
{
    // The Cpp editor plugin, which is loaded later on, registers the Cpp mime types,
    // so, apply settings here
    m_fileSettings->fromSettings(Core::ICore::instance()->settings());
    if (!m_fileSettings->applySuffixesToMimeDB())
        qWarning("Unable to apply cpp suffixes to mime database (cpp mime types not found).\n");
}

ExtensionSystem::IPlugin::ShutdownFlag CppToolsPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

void CppToolsPlugin::switchHeaderSource()
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->currentEditor();
    QString otherFile = correspondingHeaderOrSource(editor->file()->fileName());
    if (!otherFile.isEmpty())
        editorManager->openEditor(otherFile);
}

static QStringList findFilesInProject(const QString &name,
                                   const ProjectExplorer::Project *project)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << name << project;

    if (!project)
        return QStringList();

    QString pattern = QString(1, QLatin1Char('/'));
    pattern += name;
    const QStringList projectFiles = project->files(ProjectExplorer::Project::AllFiles);
    const QStringList::const_iterator pcend = projectFiles.constEnd();
    QStringList candidateList;
    for (QStringList::const_iterator it = projectFiles.constBegin(); it != pcend; ++it) {
        if (it->endsWith(pattern))
            candidateList.append(*it);
    }
    return candidateList;
}

// Figure out file type
enum FileType {
    HeaderFile,
    C_SourceFile,
    CPP_SourceFile,
    ObjectiveCPP_SourceFile,
    UnknownType
};

static inline FileType fileType(const Core::MimeDatabase *mimeDatase, const  QFileInfo & fi)
{
    const Core::MimeType mimeType = mimeDatase->findByFile(fi);
    if (!mimeType)
        return UnknownType;
    const QString typeName = mimeType.type();
    if (typeName == QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE))
        return C_SourceFile;
    if (typeName == QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE))
        return CPP_SourceFile;
    if (typeName == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE))
        return ObjectiveCPP_SourceFile;
    if (typeName == QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE)
        || typeName == QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE))
        return HeaderFile;
    return UnknownType;
}

// Return the suffixes that should be checked when trying to find a
// source belonging to a header and vice versa
static QStringList matchingCandidateSuffixes(const Core::MimeDatabase *mimeDatase, FileType type)
{
    switch (type) {
    case UnknownType:
        break;
    case HeaderFile: // Note that C/C++ headers are undistinguishable
        return mimeDatase->findByType(QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE)).suffixes()
               + mimeDatase->findByType(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)).suffixes()
               + mimeDatase->findByType(QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)).suffixes();
    case C_SourceFile:
        return mimeDatase->findByType(QLatin1String(CppTools::Constants::C_HEADER_MIMETYPE)).suffixes();
    case CPP_SourceFile:
    case ObjectiveCPP_SourceFile:
        return mimeDatase->findByType(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)).suffixes();
    }
    return QStringList();
}

static QStringList baseNameWithAllSuffixes(const QString &baseName, const QStringList &suffixes)
{
    QStringList result;
    const QChar dot = QLatin1Char('.');
    foreach (const QString &suffix, suffixes) {
        QString fileName = baseName;
        fileName += dot;
        fileName += suffix;
        result += fileName;
    }
    return result;
}

static int commonStringLength(const QString &s1, const QString &s2)
{
    int length = qMin(s1.length(), s2.length());
    for (int i = 0; i < length; ++i)
        if (s1[i] != s2[i])
            return i;
    return length;
}

QString CppToolsPlugin::correspondingHeaderOrSourceI(const QString &fileName) const
{
    const QFileInfo fi(fileName);
    if (m_headerSourceMapping.contains(fi.absoluteFilePath()))
        return m_headerSourceMapping.value(fi.absoluteFilePath());

    const Core::ICore *core = Core::ICore::instance();
    const Core::MimeDatabase *mimeDatase = core->mimeDatabase();
    ProjectExplorer::ProjectExplorerPlugin *explorer =
       ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *project = (explorer ? explorer->currentProject() : 0);

    const FileType type = fileType(mimeDatase, fi);

    if (debug)
        qDebug() << Q_FUNC_INFO << fileName <<  type;

    if (type == UnknownType)
        return QString();

    const QString baseName = fi.completeBaseName();
    const QString privateHeaderSuffix = QLatin1String("_p");
    const QStringList suffixes = matchingCandidateSuffixes(mimeDatase, type);

    QStringList candidateFileNames = baseNameWithAllSuffixes(baseName, suffixes);
    if (type == HeaderFile) {
        if (baseName.endsWith(privateHeaderSuffix)) {
            QString sourceBaseName = baseName;
            sourceBaseName.truncate(sourceBaseName.size() - privateHeaderSuffix.size());
            candidateFileNames += baseNameWithAllSuffixes(sourceBaseName, suffixes);
        }
    } else {
        QString privateHeaderBaseName = baseName;
        privateHeaderBaseName.append(privateHeaderSuffix);
        candidateFileNames += baseNameWithAllSuffixes(privateHeaderBaseName, suffixes);
    }

    const QDir absoluteDir = fi.absoluteDir();

    // Try to find a file in the same directory first
    foreach (const QString &candidateFileName, candidateFileNames) {
        const QFileInfo candidateFi(absoluteDir, candidateFileName);
        if (candidateFi.isFile()) {
            m_headerSourceMapping[fi.absoluteFilePath()] = candidateFi.absoluteFilePath();
            m_headerSourceMapping[candidateFi.absoluteFilePath()] = fi.absoluteFilePath();
            return candidateFi.absoluteFilePath();
        }
    }

    // Find files in the project
    if (project) {
        QString bestFileName;
        int compareValue = 0;
        foreach (const QString &candidateFileName, candidateFileNames) {
            const QStringList projectFiles = findFilesInProject(candidateFileName, project);
            // Find the file having the most common path with fileName
            foreach (const QString projectFile, projectFiles) {
                int value = commonStringLength(fileName, projectFile);
                if (value > compareValue) {
                    compareValue = value;
                    bestFileName = projectFile;
                }
            }
        }
        if (!bestFileName.isEmpty()) {
            const QFileInfo candidateFi(bestFileName);
            Q_ASSERT(candidateFi.isFile());
            m_headerSourceMapping[fi.absoluteFilePath()] = candidateFi.absoluteFilePath();
            m_headerSourceMapping[candidateFi.absoluteFilePath()] = fi.absoluteFilePath();
            return candidateFi.absoluteFilePath();
        }
    }

    return QString();
}

QString CppToolsPlugin::correspondingHeaderOrSource(const QString &fileName)
{
    const QString rc = m_instance->correspondingHeaderOrSourceI(fileName);
    if (debug)
        qDebug() << Q_FUNC_INFO << fileName << rc;
    return rc;
}

Q_EXPORT_PLUGIN(CppToolsPlugin)
