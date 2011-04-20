/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
#include "cppclassesfilter.h"
#include "cppcodecompletion.h"
#include "cppfunctionsfilter.h"
#include "cppcurrentdocumentfilter.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "cpplocatorfilter.h"
#include "symbolsfindfilter.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/filemanager.h>
#include <texteditor/texteditorsettings.h>
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

CppToolsPlugin *CppToolsPlugin::m_instance = 0;

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

    // Objects
    m_modelManager = new CppModelManager(this);
    Core::VcsManager *vcsManager = core->vcsManager();
    Core::FileManager *fileManager = core->fileManager();
    connect(vcsManager, SIGNAL(repositoryChanged(QString)),
            m_modelManager, SLOT(updateModifiedSourceFiles()));
    connect(fileManager, SIGNAL(filesChangedInternally(QStringList)),
            m_modelManager, SLOT(updateSourceFiles(QStringList)));
    addAutoReleasedObject(m_modelManager);

    CppCodeCompletion *completion = new CppCodeCompletion(m_modelManager);
    addAutoReleasedObject(completion);

    addAutoReleasedObject(new CppLocatorFilter(m_modelManager));
    addAutoReleasedObject(new CppClassesFilter(m_modelManager));
    addAutoReleasedObject(new CppFunctionsFilter(m_modelManager));
    addAutoReleasedObject(new CppCurrentDocumentFilter(m_modelManager, core->editorManager()));
    addAutoReleasedObject(new CompletionSettingsPage);
    addAutoReleasedObject(new CppFileSettingsPage(m_fileSettings));
    addAutoReleasedObject(new SymbolsFindFilter(m_modelManager));

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

    // Set completion settings and keep them up to date
    TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
    completion->setCompletionSettings(textEditorSettings->completionSettings());
    connect(textEditorSettings, SIGNAL(completionSettingsChanged(TextEditor::CompletionSettings)),
            completion, SLOT(setCompletionSettings(TextEditor::CompletionSettings)));

    return true;
}

void CppToolsPlugin::extensionsInitialized()
{
    // The Cpp editor plugin, which is loaded later on, registers the Cpp mime types,
    // so, apply settings here
    m_fileSettings->fromSettings(Core::ICore::instance()->settings());
    if (!m_fileSettings->applySuffixesToMimeDB())
        qWarning("Unable to apply cpp suffixes to mime database (cpp mime types not found).\n");

    // Initialize header suffixes
    const Core::MimeDatabase *mimeDatabase = Core::ICore::instance()->mimeDatabase();
    const Core::MimeType mimeType = mimeDatabase->findByType(QLatin1String("text/x-c++hdr"));
    m_modelManager->setHeaderSuffixes(mimeType.suffixes());
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

QFileInfo CppToolsPlugin::findFile(const QDir &dir, const QString &name,
                                   const ProjectExplorer::Project *project) const
{
    if (debug)
        qDebug() << Q_FUNC_INFO << dir << name;

    QFileInfo fileInSameDir(dir, name);
    if (project && !fileInSameDir.isFile()) {
        QString pattern = QString(1, QLatin1Char('/'));
        pattern += name;
        const QStringList projectFiles = project->files(ProjectExplorer::Project::AllFiles);
        const QStringList::const_iterator pcend = projectFiles.constEnd();
        for (QStringList::const_iterator it = projectFiles.constBegin(); it != pcend; ++it)
            if (it->endsWith(pattern))
                return QFileInfo(*it);
        return QFileInfo();
    }
    return fileInSameDir;
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

QString CppToolsPlugin::correspondingHeaderOrSourceI(const QString &fileName) const
{
    const Core::ICore *core = Core::ICore::instance();
    const Core::MimeDatabase *mimeDatase = core->mimeDatabase();
    ProjectExplorer::ProjectExplorerPlugin *explorer =
       ProjectExplorer::ProjectExplorerPlugin::instance();
    ProjectExplorer::Project *project = (explorer ? explorer->currentProject() : 0);

    const QFileInfo fi(fileName);
    const FileType type = fileType(mimeDatase, fi);

    if (debug)
        qDebug() << Q_FUNC_INFO << fileName <<  type;

    if (type == UnknownType)
        return QString();

    const QDir absoluteDir = fi.absoluteDir();
    const QString baseName = fi.completeBaseName();
    const QStringList suffixes = matchingCandidateSuffixes(mimeDatase, type);

    const QString privateHeaderSuffix = QLatin1String("_p");
    const QChar dot = QLatin1Char('.');
    // Check base matches 'source.h'-> 'source.cpp' and vice versa
    const QStringList::const_iterator scend = suffixes.constEnd();
    for (QStringList::const_iterator it = suffixes.constBegin(); it != scend; ++it) {
        QString candidate = baseName;
        candidate += dot;
        candidate += *it;
        const QFileInfo candidateFi = findFile(absoluteDir, candidate, project);
        if (candidateFi.isFile())
            return candidateFi.absoluteFilePath();
    }
    if (type == HeaderFile) {
        // 'source_p.h': try 'source.cpp'
        if (baseName.endsWith(privateHeaderSuffix)) {
            QString sourceBaseName = baseName;
            sourceBaseName.truncate(sourceBaseName.size() - privateHeaderSuffix.size());
            for (QStringList::const_iterator it = suffixes.constBegin(); it != scend; ++it) {
                QString candidate = sourceBaseName;
                candidate += dot;
                candidate += *it;
                const QFileInfo candidateFi = findFile(absoluteDir, candidate, project);
                if (candidateFi.isFile())
                    return candidateFi.absoluteFilePath();
            }
        }
    } else {
        // 'source.cpp': try 'source_p.h'
        const QStringList::const_iterator scend = suffixes.constEnd();
        for (QStringList::const_iterator it = suffixes.constBegin(); it != scend; ++it) {
            QString candidate = baseName;
            candidate += privateHeaderSuffix;
            candidate += dot;
            candidate += *it;
            const QFileInfo candidateFi = findFile(absoluteDir, candidate, project);
            if (candidateFi.isFile())
                return candidateFi.absoluteFilePath();
        }
    }
    return QString();
}

QString CppToolsPlugin::correspondingHeaderOrSource(const QString &fileName) const
{
    const QString rc = correspondingHeaderOrSourceI(fileName);
    if (debug)
        qDebug() << Q_FUNC_INFO << fileName << rc;
    return rc;
}

Q_EXPORT_PLUGIN(CppToolsPlugin)
