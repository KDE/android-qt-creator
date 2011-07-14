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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include <cplusplus/pp.h>
#include <cplusplus/Overview.h>

#include "cppmodelmanager.h"
#include "abstracteditorsupport.h"
#ifndef ICHECK_BUILD
#  include "cpptoolsconstants.h"
#  include "cpptoolseditorsupport.h"
#  include "cppfindreferences.h"
#endif

#include <functional>
#include <QtCore/QtConcurrentRun>
#ifndef ICHECK_BUILD
#  include <QtCore/QFutureSynchronizer>
#  include <qtconcurrent/runextensions.h>
#  include <texteditor/itexteditor.h>
#  include <texteditor/basetexteditor.h>
#  include <projectexplorer/project.h>
#  include <projectexplorer/projectexplorer.h>
#  include <projectexplorer/projectexplorerconstants.h>
#  include <projectexplorer/session.h>
#  include <coreplugin/icore.h>
#  include <coreplugin/mimedatabase.h>
#  include <coreplugin/editormanager/editormanager.h>
#  include <coreplugin/progressmanager/progressmanager.h>
#  include <extensionsystem/pluginmanager.h>
#else
#  include <QtCore/QDir>
#endif

#include <utils/qtcassert.h>

#include <TranslationUnit.h>
#include <AST.h>
#include <Scope.h>
#include <Literals.h>
#include <Symbols.h>
#include <Names.h>
#include <NameVisitor.h>
#include <TypeVisitor.h>
#include <ASTVisitor.h>
#include <Lexer.h>
#include <Token.h>
#include <Parser.h>
#include <Control.h>
#include <CoreTypes.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtConcurrentMap>

#include <QtGui/QTextBlock>

#include <iostream>
#include <sstream>

using namespace CppTools;
using namespace CppTools::Internal;
using namespace CPlusPlus;

#if defined(QTCREATOR_WITH_DUMP_AST) && defined(Q_CC_GNU)

#include <cxxabi.h>

class DumpAST: protected ASTVisitor
{
public:
    int depth;

    DumpAST(Control *control)
        : ASTVisitor(control), depth(0)
    { }

    void operator()(AST *ast)
    { accept(ast); }

protected:
    virtual bool preVisit(AST *ast)
    {
        std::ostringstream s;
        PrettyPrinter pp(control(), s);
        pp(ast);
        QString code = QString::fromStdString(s.str());
        code.replace('\n', ' ');
        code.replace(QRegExp("\\s+"), " ");

        const char *name = abi::__cxa_demangle(typeid(*ast).name(), 0, 0, 0) + 11;

        QByteArray ind(depth, ' ');
        ind += name;

        printf("%-40s %s\n", ind.constData(), qPrintable(code));
        ++depth;
        return true;
    }

    virtual void postVisit(AST *)
    { --depth; }
};

#endif // QTCREATOR_WITH_DUMP_AST

static const char pp_configuration_file[] = "<configuration>";

static const char pp_configuration[] =
    "# 1 \"<configuration>\"\n"
    "#define __cplusplus 1\n"
    "#define __extension__\n"
    "#define __context__\n"
    "#define __range__\n"
    "#define   restrict\n"
    "#define __restrict\n"
    "#define __restrict__\n"

    "#define __complex__\n"
    "#define __imag__\n"
    "#define __real__\n"

    "#define __builtin_va_arg(a,b) ((b)0)\n"

    // ### add macros for win32
    "#define __cdecl\n"
    "#define __stdcall\n"
    "#define QT_WA(x) x\n"
    "#define API\n"
    "#define WINAPI\n"
    "#define CALLBACK\n"
    "#define STDMETHODCALLTYPE\n"
    "#define __RPC_FAR\n"
    "#define APIENTRY\n"
    "#define __declspec(a)\n"
    "#define STDMETHOD(method) virtual HRESULT STDMETHODCALLTYPE method\n";

#ifndef ICHECK_BUILD
CppPreprocessor::CppPreprocessor(QPointer<CppModelManager> modelManager)
    : snapshot(modelManager->snapshot()),
      m_modelManager(modelManager),
      preprocess(this, &env),
      m_revision(0)
{ }

#else

CppPreprocessor::CppPreprocessor(QPointer<CPlusPlus::ParseManager> modelManager)
    : preprocess(this, &env),
      m_revision(0)
{
}
#endif

CppPreprocessor::~CppPreprocessor()
{ }

void CppPreprocessor::setRevision(unsigned revision)
{ m_revision = revision; }

void CppPreprocessor::setWorkingCopy(const CppModelManagerInterface::WorkingCopy &workingCopy)
{ m_workingCopy = workingCopy; }

void CppPreprocessor::setIncludePaths(const QStringList &includePaths)
{
    m_includePaths.clear();

    for (int i = 0; i < includePaths.size(); ++i) {
        const QString &path = includePaths.at(i);

#ifdef Q_OS_DARWIN
        if (i + 1 < includePaths.size() && path.endsWith(QLatin1String(".framework/Headers"))) {
            const QFileInfo pathInfo(path);
            const QFileInfo frameworkFileInfo(pathInfo.path());
            const QString frameworkName = frameworkFileInfo.baseName();

            const QFileInfo nextIncludePath = includePaths.at(i + 1);
            if (nextIncludePath.fileName() == frameworkName) {
                // We got a QtXXX.framework/Headers followed by $QTDIR/include/QtXXX.
                // In this case we prefer to include files from $QTDIR/include/QtXXX.
                continue;
            }
        }
        m_includePaths.append(path);
#else
        m_includePaths.append(path);
#endif
    }
}

void CppPreprocessor::setFrameworkPaths(const QStringList &frameworkPaths)
{
    m_frameworkPaths.clear();

    foreach (const QString &frameworkPath, frameworkPaths) {
        addFrameworkPath(frameworkPath);
    }
}

// Add the given framework path, and expand private frameworks.
//
// Example:
//  <framework-path>/ApplicationServices.framework
// has private frameworks in:
//  <framework-path>/ApplicationServices.framework/Frameworks
// if the "Frameworks" folder exists inside the top level framework.
void CppPreprocessor::addFrameworkPath(const QString &frameworkPath)
{
    // The algorithm below is a bit too eager, but that's because we're not getting
    // in the frameworks we're linking against. If we would have that, then we could
    // add only those private frameworks.
    if (!m_frameworkPaths.contains(frameworkPath)) {
        m_frameworkPaths.append(frameworkPath);
    }

    const QDir frameworkDir(frameworkPath);
    const QStringList filter = QStringList() << QLatin1String("*.framework");
    foreach (const QFileInfo &framework, frameworkDir.entryInfoList(filter)) {
        if (!framework.isDir())
            continue;
        const QFileInfo privateFrameworks(framework.absoluteFilePath(), QLatin1String("Frameworks"));
        if (privateFrameworks.exists() && privateFrameworks.isDir()) {
            addFrameworkPath(privateFrameworks.absoluteFilePath());
        }
    }
}

void CppPreprocessor::setProjectFiles(const QStringList &files)
{ m_projectFiles = files; }

void CppPreprocessor::setTodo(const QStringList &files)
{ m_todo = QSet<QString>::fromList(files); }

#ifndef ICHECK_BUILD
namespace {
class Process: public std::unary_function<Document::Ptr, void>
{
    QPointer<CppModelManager> _modelManager;
    Snapshot _snapshot;
    Document::Ptr _doc;
    Document::CheckMode _mode;

public:
    Process(QPointer<CppModelManager> modelManager,
            Document::Ptr doc,
            const Snapshot &snapshot,
            const CppModelManager::WorkingCopy &workingCopy)
        : _modelManager(modelManager),
          _snapshot(snapshot),
          _doc(doc),
          _mode(Document::FastCheck)
    {

        if (workingCopy.contains(_doc->fileName()))
            _mode = Document::FullCheck;
    }

    void operator()()
    {
        _doc->check(_mode);
        _doc->findExposedQmlTypes();
        _doc->releaseSource();
        _doc->releaseTranslationUnit();

        if (_mode == Document::FastCheck)
            _doc->control()->squeeze();

        if (_modelManager)
            _modelManager->emitDocumentUpdated(_doc); // ### TODO: compress
    }
};
} // end of anonymous namespace
#endif

void CppPreprocessor::run(const QString &fileName)
{
    QString absoluteFilePath = fileName;
    sourceNeeded(absoluteFilePath, IncludeGlobal, /*line = */ 0);
}

void CppPreprocessor::resetEnvironment()
{
    env.reset();
    m_processed.clear();
}

bool CppPreprocessor::includeFile(const QString &absoluteFilePath, QString *result, unsigned *revision)
{
    if (absoluteFilePath.isEmpty() || m_included.contains(absoluteFilePath))
        return true;

    if (m_workingCopy.contains(absoluteFilePath)) {
        m_included.insert(absoluteFilePath);
        const QPair<QString, unsigned> r = m_workingCopy.get(absoluteFilePath);
        *result = r.first;
        *revision = r.second;
        return true;
    }

    QFileInfo fileInfo(absoluteFilePath);
    if (! fileInfo.isFile())
        return false;

    QFile file(absoluteFilePath);
    if (file.open(QFile::ReadOnly)) {
        m_included.insert(absoluteFilePath);
        QTextStream stream(&file);
        const QString contents = stream.readAll();
        *result = contents.toUtf8();
        file.close();
        return true;
    }

    return false;
}

QString CppPreprocessor::tryIncludeFile(QString &fileName, IncludeType type, unsigned *revision)
{
    if (type == IncludeGlobal) {
        const QString fn = m_fileNameCache.value(fileName);

        if (! fn.isEmpty()) {
            fileName = fn;

            if (revision)
                *revision = 0;

            return QString();
        }
    }

    const QString originalFileName = fileName;
    const QString contents = tryIncludeFile_helper(fileName, type, revision);
    if (type == IncludeGlobal)
        m_fileNameCache.insert(originalFileName, fileName);
    return contents;
}

static inline void appendDirSeparatorIfNeeded(QString &path)
{
    if (!path.endsWith(QLatin1Char('/'), Qt::CaseInsensitive))
        path += QLatin1Char('/');
}

QString CppPreprocessor::tryIncludeFile_helper(QString &fileName, IncludeType type, unsigned *revision)
{
    QFileInfo fileInfo(fileName);
    if (fileName == QLatin1String(pp_configuration_file) || fileInfo.isAbsolute()) {
        QString contents;
        includeFile(fileName, &contents, revision);
        return contents;
    }

    if (type == IncludeLocal && m_currentDoc) {
        QFileInfo currentFileInfo(m_currentDoc->fileName());
        QString path = currentFileInfo.absolutePath();
        appendDirSeparatorIfNeeded(path);
        path += fileName;
        path = QDir::cleanPath(path);
        QString contents;
        if (includeFile(path, &contents, revision)) {
            fileName = path;
            return contents;
        }
    }

    foreach (const QString &includePath, m_includePaths) {
        QString path = includePath;
        appendDirSeparatorIfNeeded(path);
        path += fileName;
        path = QDir::cleanPath(path);
        QString contents;
        if (includeFile(path, &contents, revision)) {
            fileName = path;
            return contents;
        }
    }

    // look in the system include paths
    foreach (const QString &includePath, m_systemIncludePaths) {
        QString path = includePath;
        appendDirSeparatorIfNeeded(path);
        path += fileName;
        path = QDir::cleanPath(path);
        QString contents;
        if (includeFile(path, &contents, revision)) {
            fileName = path;
            return contents;
        }
    }

    int index = fileName.indexOf(QLatin1Char('/'));
    if (index != -1) {
        QString frameworkName = fileName.left(index);
        QString name = fileName.mid(index + 1);

        foreach (const QString &frameworkPath, m_frameworkPaths) {
            QString path = frameworkPath;
            appendDirSeparatorIfNeeded(path);
            path += frameworkName;
            path += QLatin1String(".framework/Headers/");
            path += name;
            path = QDir::cleanPath(path);
            QString contents;
            if (includeFile(path, &contents, revision)) {
                fileName = path;
                return contents;
            }
        }
    }

    QString path = fileName;
    if (path.at(0) != QLatin1Char('/'))
        path.prepend(QLatin1Char('/'));

    foreach (const QString &projectFile, m_projectFiles) {
        if (projectFile.endsWith(path)) {
            fileName = projectFile;
            QString contents;
            includeFile(fileName, &contents, revision);
            return contents;
        }
    }

    //qDebug() << "**** file" << fileName << "not found!";
    return QString();
}

void CppPreprocessor::macroAdded(const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->appendMacro(macro);
}

void CppPreprocessor::passedMacroDefinitionCheck(unsigned offset, const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addMacroUse(macro, offset, macro.name().length(), env.currentLine,
                              QVector<MacroArgumentReference>(), true);
}

void CppPreprocessor::failedMacroDefinitionCheck(unsigned offset, const QByteArray &name)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addUndefinedMacroUse(name, offset);
}

void CppPreprocessor::startExpandingMacro(unsigned offset,
                                          const Macro &macro,
                                          const QByteArray &originalText,
                                          bool inCondition,
                                          const QVector<MacroArgumentReference> &actuals)
{
    if (! m_currentDoc)
        return;

    //qDebug() << "start expanding:" << macro.name() << "text:" << originalText;
    m_currentDoc->addMacroUse(macro, offset, originalText.length(), env.currentLine,
                              actuals, inCondition);
}

void CppPreprocessor::stopExpandingMacro(unsigned, const Macro &)
{
    if (! m_currentDoc)
        return;

    //qDebug() << "stop expanding:" << macro.name;
}

void CppPreprocessor::mergeEnvironment(Document::Ptr doc)
{
    if (! doc)
        return;

    const QString fn = doc->fileName();

    if (m_processed.contains(fn))
        return;

    m_processed.insert(fn);

    foreach (const Document::Include &incl, doc->includes()) {
        QString includedFile = incl.fileName();

        if (Document::Ptr includedDoc = snapshot.document(includedFile))
            mergeEnvironment(includedDoc);
        else
            run(includedFile);
    }

    env.addMacros(doc->definedMacros());
}

void CppPreprocessor::startSkippingBlocks(unsigned offset)
{
    //qDebug() << "start skipping blocks:" << offset;
    if (m_currentDoc)
        m_currentDoc->startSkippingBlocks(offset);
}

void CppPreprocessor::stopSkippingBlocks(unsigned offset)
{
    //qDebug() << "stop skipping blocks:" << offset;
    if (m_currentDoc)
        m_currentDoc->stopSkippingBlocks(offset);
}

void CppPreprocessor::sourceNeeded(QString &fileName, IncludeType type, unsigned line)
{
    if (fileName.isEmpty())
        return;

    unsigned editorRevision = 0;
    QString contents = tryIncludeFile(fileName, type, &editorRevision);
    fileName = QDir::cleanPath(fileName);
    if (m_currentDoc) {
        m_currentDoc->addIncludeFile(fileName, line);

        if (contents.isEmpty() && ! QFileInfo(fileName).isAbsolute()) {
            QString msg = QCoreApplication::translate(
                    "CppPreprocessor", "%1: No such file or directory").arg(fileName);

            Document::DiagnosticMessage d(Document::DiagnosticMessage::Warning,
                                          m_currentDoc->fileName(),
                                          env.currentLine, /*column = */ 0,
                                          msg);

            m_currentDoc->addDiagnosticMessage(d);

            //qWarning() << "file not found:" << fileName << m_currentDoc->fileName() << env.current_line;
        }
    }

    //qDebug() << "parse file:" << fileName << "contents:" << contents.size();

    Document::Ptr doc = snapshot.document(fileName);
    if (doc) {
        mergeEnvironment(doc);
        return;
    }

    doc = Document::create(fileName);
    doc->setRevision(m_revision);
    doc->setEditorRevision(editorRevision);

    QFileInfo info(fileName);
    if (info.exists())
        doc->setLastModified(info.lastModified());

    Document::Ptr previousDoc = switchDocument(doc);

    const QByteArray preprocessedCode = preprocess(fileName, contents);

    doc->setSource(preprocessedCode);
    doc->tokenize();

    snapshot.insert(doc);
    m_todo.remove(fileName);

#ifndef ICHECK_BUILD
    Process process(m_modelManager, doc, snapshot, m_workingCopy);

    process();

    (void) switchDocument(previousDoc);
#else
    doc->releaseSource();
    Document::CheckMode mode = Document::FastCheck;
    mode = Document::FullCheck;
    doc->parse();
    doc->check(mode);

    (void) switchDocument(previousDoc);
#endif
}

Document::Ptr CppPreprocessor::switchDocument(Document::Ptr doc)
{
    Document::Ptr previousDoc = m_currentDoc;
    m_currentDoc = doc;
    return previousDoc;
}

#ifndef ICHECK_BUILD
void CppModelManager::updateModifiedSourceFiles()
{
    const Snapshot snapshot = this->snapshot();
    QStringList sourceFiles;

    foreach (const Document::Ptr doc, snapshot) {
        const QDateTime lastModified = doc->lastModified();

        if (! lastModified.isNull()) {
            QFileInfo fileInfo(doc->fileName());

            if (fileInfo.exists() && fileInfo.lastModified() != lastModified)
                sourceFiles.append(doc->fileName());
        }
    }

    updateSourceFiles(sourceFiles);
}

CppModelManager *CppModelManager::instance()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    return pluginManager->getObject<CppModelManager>();
}


/*!
    \class CppTools::CppModelManager
    \brief The CppModelManager keeps track of one CppCodeModel instance
           for each project and all related CppCodeModelPart instances.

    It also takes care of updating the code models when C++ files are
    modified within Qt Creator.
*/

CppModelManager::CppModelManager(QObject *parent)
    : CppModelManagerInterface(parent)
{
    m_findReferences = new CppFindReferences(this);
    m_indexerEnabled = qgetenv("QTCREATOR_NO_CODE_INDEXER").isNull();

    m_revision = 0;
    m_synchronizer.setCancelOnWait(true);

    m_core = Core::ICore::instance(); // FIXME
    m_dirty = true;

    ProjectExplorer::ProjectExplorerPlugin *pe =
       ProjectExplorer::ProjectExplorerPlugin::instance();

    QTC_ASSERT(pe, return);

    ProjectExplorer::SessionManager *session = pe->session();
    QTC_ASSERT(session, return);

    m_updateEditorSelectionsTimer = new QTimer(this);
    m_updateEditorSelectionsTimer->setInterval(500);
    m_updateEditorSelectionsTimer->setSingleShot(true);
    connect(m_updateEditorSelectionsTimer, SIGNAL(timeout()),
            this, SLOT(updateEditorSelections()));

    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(onProjectAdded(ProjectExplorer::Project*)));

    connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project *)),
            this, SLOT(onAboutToRemoveProject(ProjectExplorer::Project *)));

    connect(session, SIGNAL(aboutToUnloadSession()),
            this, SLOT(onAboutToUnloadSession()));

    qRegisterMetaType<CPlusPlus::Document::Ptr>("CPlusPlus::Document::Ptr");

    // thread connections
    connect(this, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));

    // Listen for editor closed and opened events so that we can keep track of changing files
    connect(m_core->editorManager(), SIGNAL(editorOpened(Core::IEditor *)),
        this, SLOT(editorOpened(Core::IEditor *)));

    connect(m_core->editorManager(), SIGNAL(editorAboutToClose(Core::IEditor *)),
        this, SLOT(editorAboutToClose(Core::IEditor *)));
}

CppModelManager::~CppModelManager()
{ }

Snapshot CppModelManager::snapshot() const
{
    QMutexLocker locker(&protectSnapshot);
    return m_snapshot;
}

void CppModelManager::ensureUpdated()
{
    QMutexLocker locker(&mutex);
    if (! m_dirty)
        return;

    m_projectFiles = internalProjectFiles();
    m_includePaths = internalIncludePaths();
    m_frameworkPaths = internalFrameworkPaths();
    m_definedMacros = internalDefinedMacros();
    m_dirty = false;
}

QStringList CppModelManager::internalProjectFiles() const
{
    QStringList files;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        ProjectInfo pinfo = it.value();
        files += pinfo.sourceFiles;
    }
    files.removeDuplicates();
    return files;
}

QStringList CppModelManager::internalIncludePaths() const
{
    QStringList includePaths;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        ProjectInfo pinfo = it.value();
        includePaths += pinfo.includePaths;
    }
    includePaths.removeDuplicates();
    return includePaths;
}

QStringList CppModelManager::internalFrameworkPaths() const
{
    QStringList frameworkPaths;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        ProjectInfo pinfo = it.value();
        frameworkPaths += pinfo.frameworkPaths;
    }
    frameworkPaths.removeDuplicates();
    return frameworkPaths;
}

QByteArray CppModelManager::internalDefinedMacros() const
{
    QByteArray macros;
    QMapIterator<ProjectExplorer::Project *, ProjectInfo> it(m_projects);
    while (it.hasNext()) {
        it.next();
        ProjectInfo pinfo = it.value();
        macros += pinfo.defines;
    }
    return macros;
}

void CppModelManager::addEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_addtionalEditorSupport.insert(editorSupport);
}

void CppModelManager::removeEditorSupport(AbstractEditorSupport *editorSupport)
{
    m_addtionalEditorSupport.remove(editorSupport);
}

QList<int> CppModelManager::references(CPlusPlus::Symbol *symbol, const LookupContext &context)
{
    return m_findReferences->references(symbol, context);
}

void CppModelManager::findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context)
{
    if (symbol->identifier())
        m_findReferences->findUsages(symbol, context);
}

void CppModelManager::renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                                   const QString &replacement)
{
    if (symbol->identifier())
        m_findReferences->renameUsages(symbol, context, replacement);
}

void CppModelManager::findMacroUsages(const CPlusPlus::Macro &macro)
{
    m_findReferences->findMacroUses(macro);
}

CppModelManager::WorkingCopy CppModelManager::buildWorkingCopyList()
{
    WorkingCopy workingCopy;
    QMapIterator<TextEditor::ITextEditor *, CppEditorSupport *> it(m_editorSupport);
    while (it.hasNext()) {
        it.next();
        TextEditor::ITextEditor *textEditor = it.key();
        CppEditorSupport *editorSupport = it.value();
        QString fileName = textEditor->file()->fileName();
        workingCopy.insert(fileName, editorSupport->contents(), editorSupport->editorRevision());
    }

    QSetIterator<AbstractEditorSupport *> jt(m_addtionalEditorSupport);
    while (jt.hasNext()) {
        AbstractEditorSupport *es =  jt.next();
        workingCopy.insert(es->fileName(), es->contents());
    }

    // add the project configuration file
    QByteArray conf(pp_configuration);
    conf += definedMacros();
    workingCopy.insert(pp_configuration_file, conf);

    return workingCopy;
}

CppModelManager::WorkingCopy CppModelManager::workingCopy() const
{
    return const_cast<CppModelManager *>(this)->buildWorkingCopyList();
}

QFuture<void> CppModelManager::updateSourceFiles(const QStringList &sourceFiles)
{ return refreshSourceFiles(sourceFiles); }

QList<CppModelManager::ProjectInfo> CppModelManager::projectInfos() const
{
    QMutexLocker locker(&mutex);

    return m_projects.values();
}

CppModelManager::ProjectInfo CppModelManager::projectInfo(ProjectExplorer::Project *project) const
{
    QMutexLocker locker(&mutex);

    return m_projects.value(project, ProjectInfo(project));
}

void CppModelManager::updateProjectInfo(const ProjectInfo &pinfo)
{
    QMutexLocker locker(&mutex);

    if (! pinfo.isValid())
        return;

    m_projects.insert(pinfo.project, pinfo);
    m_dirty = true;
}

QFuture<void> CppModelManager::refreshSourceFiles(const QStringList &sourceFiles)
{
    if (! sourceFiles.isEmpty() && m_indexerEnabled) {
        const WorkingCopy workingCopy = buildWorkingCopyList();

        CppPreprocessor *preproc = new CppPreprocessor(this);
        preproc->setRevision(++m_revision);
        preproc->setProjectFiles(projectFiles());
        preproc->setIncludePaths(includePaths());
        preproc->setFrameworkPaths(frameworkPaths());
        preproc->setWorkingCopy(workingCopy);

        QFuture<void> result = QtConcurrent::run(&CppModelManager::parse,
                                                 preproc, sourceFiles);

        if (m_synchronizer.futures().size() > 10) {
            QList<QFuture<void> > futures = m_synchronizer.futures();

            m_synchronizer.clearFutures();

            foreach (const QFuture<void> &future, futures) {
                if (! (future.isFinished() || future.isCanceled()))
                    m_synchronizer.addFuture(future);
            }
        }

        m_synchronizer.addFuture(result);

        if (sourceFiles.count() > 1) {
            m_core->progressManager()->addTask(result, tr("Parsing"),
                            CppTools::Constants::TASK_INDEX);
        }

        return result;
    }
    return QFuture<void>();
}

/*!
    \fn    void CppModelManager::editorOpened(Core::IEditor *editor)
    \brief If a C++ editor is opened, the model manager listens to content changes
           in order to update the CppCodeModel accordingly. It also updates the
           CppCodeModel for the first time with this editor.

    \sa    void CppModelManager::editorContentsChanged()
 */
void CppModelManager::editorOpened(Core::IEditor *editor)
{
    if (isCppEditor(editor)) {
        TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
        QTC_ASSERT(textEditor, return);

        CppEditorSupport *editorSupport = new CppEditorSupport(this);
        editorSupport->setTextEditor(textEditor);
        m_editorSupport[textEditor] = editorSupport;
    }
}

void CppModelManager::editorAboutToClose(Core::IEditor *editor)
{
    if (isCppEditor(editor)) {
        TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
        QTC_ASSERT(textEditor, return);

        CppEditorSupport *editorSupport = m_editorSupport.value(textEditor);
        m_editorSupport.remove(textEditor);
        delete editorSupport;
    }
}

bool CppModelManager::isCppEditor(Core::IEditor *editor) const
{
    return editor->context().contains(ProjectExplorer::Constants::LANG_CXX);
}

void CppModelManager::emitDocumentUpdated(Document::Ptr doc)
{
    emit documentUpdated(doc);
}

void CppModelManager::onDocumentUpdated(Document::Ptr doc)
{
    const QString fileName = doc->fileName();

    bool outdated = false;

    protectSnapshot.lock();

    Document::Ptr previous = m_snapshot.document(fileName);

    if (previous && (doc->revision() != 0 && doc->revision() < previous->revision()))
        outdated = true;
    else
        m_snapshot.insert(doc);

    protectSnapshot.unlock();

    if (outdated)
        return;

    QList<Core::IEditor *> openedEditors = m_core->editorManager()->openedEditors();
    foreach (Core::IEditor *editor, openedEditors) {
        if (editor->file()->fileName() == fileName) {
            TextEditor::ITextEditor *textEditor = qobject_cast<TextEditor::ITextEditor *>(editor);
            if (! textEditor)
                continue;

            TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(textEditor->widget());
            if (! ed)
                continue;

            QList<TextEditor::BaseTextEditorWidget::BlockRange> blockRanges;

            foreach (const Document::Block &block, doc->skippedBlocks()) {
                blockRanges.append(TextEditor::BaseTextEditorWidget::BlockRange(block.begin(), block.end()));
            }

            QList<QTextEdit::ExtraSelection> selections;

#ifdef QTCREATOR_WITH_MACRO_HIGHLIGHTING
            // set up the format for the macros
            QTextCharFormat macroFormat;
            macroFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);

            QTextCursor c = ed->textCursor();
            foreach (const Document::MacroUse &block, doc->macroUses()) {
                QTextEdit::ExtraSelection sel;
                sel.cursor = c;
                sel.cursor.setPosition(block.begin());
                sel.cursor.setPosition(block.end(), QTextCursor::KeepAnchor);
                sel.format = macroFormat;
                selections.append(sel);
            }
#endif // QTCREATOR_WITH_MACRO_HIGHLIGHTING

            // set up the format for the errors
            QTextCharFormat errorFormat;
            errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            errorFormat.setUnderlineColor(Qt::red);

            // set up the format for the warnings.
            QTextCharFormat warningFormat;
            warningFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            warningFormat.setUnderlineColor(Qt::darkYellow);

#ifdef QTCREATOR_WITH_ADVANCED_HIGHLIGHTER
            QSet<QPair<unsigned, unsigned> > lines;
            foreach (const Document::DiagnosticMessage &m, doc->diagnosticMessages()) {
                if (m.fileName() != fileName)
                    continue;

                const QPair<unsigned, unsigned> coordinates = qMakePair(m.line(), m.column());

                if (lines.contains(coordinates))
                    continue;

                lines.insert(coordinates);

                QTextEdit::ExtraSelection sel;
                if (m.isWarning())
                    sel.format = warningFormat;
                else
                    sel.format = errorFormat;

                QTextCursor c(ed->document()->findBlockByNumber(m.line() - 1));

                // ### check for generated tokens.

                int column = m.column();

                if (column > c.block().length()) {
                    column = 0;

                    const QString text = c.block().text();
                    for (int i = 0; i < text.size(); ++i) {
                        if (! text.at(i).isSpace()) {
                            ++column;
                            break;
                        }
                    }
                }

                if (column != 0)
                    --column;

                c.setPosition(c.position() + column);
                c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
                sel.cursor = c;
                selections.append(sel);
            }
#else
            QSet<int> lines;
            foreach (const Document::DiagnosticMessage &m, doc->diagnosticMessages()) {
                if (m.fileName() != fileName)
                    continue;
                else if (lines.contains(m.line()))
                    continue;

                lines.insert(m.line());

                QTextEdit::ExtraSelection sel;
                if (m.isWarning())
                    sel.format = warningFormat;
                else
                    sel.format = errorFormat;

                QTextCursor c(ed->document()->findBlockByNumber(m.line() - 1));
                const QString text = c.block().text();
                for (int i = 0; i < text.size(); ++i) {
                    if (! text.at(i).isSpace()) {
                        c.setPosition(c.position() + i);
                        break;
                    }
                }
                c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                sel.cursor = c;
                selections.append(sel);
            }
#endif
            QList<Editor> todo;
            foreach (const Editor &e, todo) {
                if (e.textEditor != textEditor)
                    todo.append(e);
            }

            Editor e;
            e.revision = ed->document()->revision();
            e.textEditor = textEditor;
            e.selections = selections;
            e.ifdefedOutBlocks = blockRanges;
            todo.append(e);
            m_todo = todo;
            postEditorUpdate();
            break;
        }
    }
}

void CppModelManager::postEditorUpdate()
{
    m_updateEditorSelectionsTimer->start(500);
}

void CppModelManager::updateEditorSelections()
{
    foreach (const Editor &ed, m_todo) {
        if (! ed.textEditor)
            continue;

        TextEditor::ITextEditor *textEditor = ed.textEditor;
        TextEditor::BaseTextEditorWidget *editor = qobject_cast<TextEditor::BaseTextEditorWidget *>(textEditor->widget());

        if (! editor)
            continue;
        else if (editor->document()->revision() != ed.revision)
            continue; // outdated

        editor->setExtraSelections(TextEditor::BaseTextEditorWidget::CodeWarningsSelection,
                                   ed.selections);

        editor->setIfdefedOutBlocks(ed.ifdefedOutBlocks);
    }

    m_todo.clear();

}

void CppModelManager::onProjectAdded(ProjectExplorer::Project *)
{
    QMutexLocker locker(&mutex);
    m_dirty = true;
}

void CppModelManager::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    do {
        QMutexLocker locker(&mutex);
        m_dirty = true;
        m_projects.remove(project);
    } while (0);

    GC();
}

void CppModelManager::onAboutToUnloadSession()
{
    if (m_core->progressManager()) {
        m_core->progressManager()->cancelTasks(CppTools::Constants::TASK_INDEX);
    }
    do {
        QMutexLocker locker(&mutex);
        m_projects.clear();
        m_dirty = true;
    } while (0);

    GC();
}

void CppModelManager::parse(QFutureInterface<void> &future,
                            CppPreprocessor *preproc,
                            QStringList files)
{
    if (files.isEmpty())
        return;

    const Core::MimeDatabase *mimeDb = Core::ICore::instance()->mimeDatabase();
    Core::MimeType cSourceTy = mimeDb->findByType(QLatin1String("text/x-csrc"));
    Core::MimeType cppSourceTy = mimeDb->findByType(QLatin1String("text/x-c++src"));
    Core::MimeType mSourceTy = mimeDb->findByType(QLatin1String("text/x-objcsrc"));

    QStringList sources;
    QStringList headers;

    QStringList suffixes = cSourceTy.suffixes();
    suffixes += cppSourceTy.suffixes();
    suffixes += mSourceTy.suffixes();

    foreach (const QString &file, files) {
        QFileInfo info(file);

        preproc->snapshot.remove(file);

        if (suffixes.contains(info.suffix()))
            sources.append(file);
        else
            headers.append(file);
    }

    const int sourceCount = sources.size();
    files = sources;
    files += headers;

    preproc->setTodo(files);

    future.setProgressRange(0, files.size());

    QString conf = QLatin1String(pp_configuration_file);

    bool processingHeaders = false;

    for (int i = 0; i < files.size(); ++i) {
        if (future.isPaused())
            future.waitForResume();

        if (future.isCanceled())
            break;

        const QString fileName = files.at(i);

        const bool isSourceFile = i < sourceCount;
        if (isSourceFile)
            (void) preproc->run(conf);

        else if (! processingHeaders) {
            (void) preproc->run(conf);

            processingHeaders = true;
        }

        preproc->run(fileName);

        future.setProgressValue(files.size() - preproc->todo().size());

        if (isSourceFile)
            preproc->resetEnvironment();
    }

    future.setProgressValue(files.size());

    delete preproc;
}

void CppModelManager::GC()
{
    protectSnapshot.lock();
    Snapshot currentSnapshot = m_snapshot;
    protectSnapshot.unlock();

    QSet<QString> processed;
    QStringList todo = projectFiles();

    while (! todo.isEmpty()) {
        QString fn = todo.last();
        todo.removeLast();

        if (processed.contains(fn))
            continue;

        processed.insert(fn);

        if (Document::Ptr doc = currentSnapshot.document(fn)) {
            todo += doc->includedFiles();
        }
    }

    QStringList removedFiles;

    Snapshot newSnapshot;
    for (Snapshot::const_iterator it = currentSnapshot.begin(); it != currentSnapshot.end(); ++it) {
        const QString fileName = it.key();

        if (processed.contains(fileName))
            newSnapshot.insert(it.value());
        else
            removedFiles.append(fileName);
    }

    emit aboutToRemoveFiles(removedFiles);

    protectSnapshot.lock();
    m_snapshot = newSnapshot;
    protectSnapshot.unlock();
}

static FullySpecifiedType stripPointerAndReference(const FullySpecifiedType &type)
{
    Type *t = type.type();
    while (t) {
        if (PointerType *ptr = t->asPointerType())
            t = ptr->elementType().type();
        else if (ReferenceType *ref = t->asReferenceType())
            t = ref->elementType().type();
        else
            break;
    }
    return FullySpecifiedType(t);
}

static QString toQmlType(const FullySpecifiedType &type)
{
    Overview overview;
    QString result = overview(stripPointerAndReference(type));
    if (result == QLatin1String("QString"))
        result = QLatin1String("string");
    return result;
}

static Class *lookupClass(const QString &expression, Scope *scope, TypeOfExpression &typeOf)
{
    QList<LookupItem> results = typeOf(expression, scope);
    Class *klass = 0;
    foreach (const LookupItem &item, results) {
        if (item.declaration()) {
            klass = item.declaration()->asClass();
            if (klass)
                return klass;
        }
    }
    return 0;
}

static void populate(LanguageUtils::FakeMetaObject::Ptr fmo, Class *klass,
                     QHash<Class *, LanguageUtils::FakeMetaObject::Ptr> *classes,
                     TypeOfExpression &typeOf)
{
    using namespace LanguageUtils;

    Overview namePrinter;

    classes->insert(klass, fmo);

    for (unsigned i = 0; i < klass->memberCount(); ++i) {
        Symbol *member = klass->memberAt(i);
        if (!member->name())
            continue;
        if (Function *func = member->type()->asFunctionType()) {
            if (!func->isSlot() && !func->isInvokable() && !func->isSignal())
                continue;
            FakeMetaMethod method(namePrinter(func->name()), toQmlType(func->returnType()));
            if (func->isSignal())
                method.setMethodType(FakeMetaMethod::Signal);
            else
                method.setMethodType(FakeMetaMethod::Slot);
            for (unsigned a = 0; a < func->argumentCount(); ++a) {
                Symbol *arg = func->argumentAt(a);
                QString name(CppModelManager::tr("unnamed"));
                if (arg->name())
                    name = namePrinter(arg->name());
                method.addParameter(name, toQmlType(arg->type()));
            }
            fmo->addMethod(method);
        }
        if (QtPropertyDeclaration *propDecl = member->asQtPropertyDeclaration()) {
            const FullySpecifiedType &type = propDecl->type();
            const bool isList = false; // ### fixme
            const bool isWritable = propDecl->flags() & QtPropertyDeclaration::WriteFunction;
            const bool isPointer = type.type() && type.type()->isPointerType();
            const int revision = 0; // ### fixme
            FakeMetaProperty property(
                        namePrinter(propDecl->name()),
                        toQmlType(type),
                        isList, isWritable, isPointer,
                        revision);
            fmo->addProperty(property);
        }
        if (QtEnum *qtEnum = member->asQtEnum()) {
            // find the matching enum
            Enum *e = 0;
            QList<LookupItem> result = typeOf(namePrinter(qtEnum->name()), klass);
            foreach (const LookupItem &item, result) {
                if (item.declaration()) {
                    e = item.declaration()->asEnum();
                    if (e)
                        break;
                }
            }
            if (!e)
                continue;

            FakeMetaEnum metaEnum(namePrinter(e->name()));
            for (unsigned j = 0; j < e->memberCount(); ++j) {
                Symbol *enumMember = e->memberAt(j);
                if (!enumMember->name())
                    continue;
                metaEnum.addKey(namePrinter(enumMember->name()), 0);
            }
            fmo->addEnum(metaEnum);
        }
    }

    // only single inheritance is supported
    if (klass->baseClassCount() > 0) {
        BaseClass *base = klass->baseClassAt(0);
        if (!base->name())
            return;

        const QString baseClassName = namePrinter(base->name());
        fmo->setSuperclassName(baseClassName);

        Class *baseClass = lookupClass(baseClassName, klass, typeOf);
        if (!baseClass)
            return;

        FakeMetaObject::Ptr baseFmo = classes->value(baseClass);
        if (!baseFmo) {
            baseFmo = FakeMetaObject::Ptr(new FakeMetaObject);
            populate(baseFmo, baseClass, classes, typeOf);
        }
    }
}

QList<LanguageUtils::FakeMetaObject::ConstPtr> CppModelManager::exportedQmlObjects(const Document::Ptr &doc) const
{
    using namespace LanguageUtils;
    QList<FakeMetaObject::ConstPtr> exportedObjects;
    QHash<Class *, FakeMetaObject::Ptr> classes;

    const QList<CPlusPlus::Document::ExportedQmlType> exported = doc->exportedQmlTypes();
    if (exported.isEmpty())
        return exportedObjects;

    TypeOfExpression typeOf;
    const Snapshot currentSnapshot = snapshot();
    typeOf.init(doc, currentSnapshot);
    foreach (const Document::ExportedQmlType &exportedType, exported) {
        FakeMetaObject::Ptr fmo(new FakeMetaObject);
        fmo->addExport(exportedType.typeName, exportedType.packageName,
                       ComponentVersion(exportedType.majorVersion, exportedType.minorVersion));
        exportedObjects += fmo;

        Class *klass = lookupClass(exportedType.typeExpression, exportedType.scope, typeOf);
        if (!klass)
            continue;

        // add the no-package export, so the cpp name can be used in properties
        Overview overview;
        fmo->addExport(overview(klass->name()), QString(), ComponentVersion());

        populate(fmo, klass, &classes, typeOf);
    }

    return exportedObjects;
}

#endif

