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

#include "externaltool.h"
#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "variablemanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/environment.h>

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtGui/QMenu>
#include <QtGui/QAction>

#include <QtDebug>

using namespace Core;
using namespace Core::Internal;

namespace {
    const char * const kExternalTool = "externaltool";
    const char * const kId = "id";
    const char * const kDescription = "description";
    const char * const kDisplayName = "displayname";
    const char * const kCategory = "category";
    const char * const kOrder = "order";
    const char * const kExecutable = "executable";
    const char * const kPath = "path";
    const char * const kArguments = "arguments";
    const char * const kInput = "input";
    const char * const kWorkingDirectory = "workingdirectory";

    const char * const kXmlLang = "xml:lang";
    const char * const kOutput = "output";
    const char * const kError = "error";
    const char * const kOutputShowInPane = "showinpane";
    const char * const kOutputReplaceSelection = "replaceselection";
    const char * const kOutputIgnore = "ignore";
    const char * const kModifiesDocument = "modifiesdocument";
    const char * const kYes = "yes";
    const char * const kNo = "no";
    const char * const kTrue= "true";
    const char * const kFalse = "false";

    const char * const kSpecialUncategorizedSetting = "SpecialEmptyCategoryForUncategorizedTools";
}

// #pragma mark -- ExternalTool

ExternalTool::ExternalTool() :
    m_displayCategory(QLatin1String("")), // difference between isNull and isEmpty
    m_order(-1),
    m_outputHandling(ShowInPane),
    m_errorHandling(ShowInPane),
    m_modifiesCurrentDocument(false)
{
}

ExternalTool::ExternalTool(const ExternalTool *other)
    : m_id(other->m_id),
      m_description(other->m_description),
      m_displayName(other->m_displayName),
      m_displayCategory(other->m_displayCategory),
      m_order(other->m_order),
      m_executables(other->m_executables),
      m_arguments(other->m_arguments),
      m_input(other->m_input),
      m_workingDirectory(other->m_workingDirectory),
      m_outputHandling(other->m_outputHandling),
      m_errorHandling(other->m_errorHandling),
      m_modifiesCurrentDocument(other->m_modifiesCurrentDocument),
      m_fileName(other->m_fileName),
      m_presetTool(other->m_presetTool)
{
}

ExternalTool &ExternalTool::operator=(const ExternalTool &other)
{
    m_id = other.m_id;
    m_description = other.m_description;
    m_displayName = other.m_displayName;
    m_displayCategory = other.m_displayCategory;
    m_order = other.m_order;
    m_executables = other.m_executables;
    m_arguments = other.m_arguments;
    m_input = other.m_input;
    m_workingDirectory = other.m_workingDirectory;
    m_outputHandling = other.m_outputHandling;
    m_errorHandling = other.m_errorHandling;
    m_modifiesCurrentDocument = other.m_modifiesCurrentDocument;
    m_fileName = other.m_fileName;
    m_presetTool = other.m_presetTool;
    return *this;
}

ExternalTool::~ExternalTool()
{
}

QString ExternalTool::id() const
{
    return m_id;
}

QString ExternalTool::description() const
{
    return m_description;
}

QString ExternalTool::displayName() const
{
    return m_displayName;
}

QString ExternalTool::displayCategory() const
{
    return m_displayCategory;
}

int ExternalTool::order() const
{
    return m_order;
}

QStringList ExternalTool::executables() const
{
    return m_executables;
}

QString ExternalTool::arguments() const
{
    return m_arguments;
}

QString ExternalTool::input() const
{
    return m_input;
}

QString ExternalTool::workingDirectory() const
{
    return m_workingDirectory;
}

ExternalTool::OutputHandling ExternalTool::outputHandling() const
{
    return m_outputHandling;
}

ExternalTool::OutputHandling ExternalTool::errorHandling() const
{
    return m_errorHandling;
}

bool ExternalTool::modifiesCurrentDocument() const
{
    return m_modifiesCurrentDocument;
}

void ExternalTool::setFileName(const QString &fileName)
{
    m_fileName = fileName;
}

void ExternalTool::setPreset(QSharedPointer<ExternalTool> preset)
{
    m_presetTool = preset;
}

QString ExternalTool::fileName() const
{
    return m_fileName;
}

QSharedPointer<ExternalTool> ExternalTool::preset() const
{
    return m_presetTool;
}

void ExternalTool::setId(const QString &id)
{
    m_id = id;
}

void ExternalTool::setDisplayCategory(const QString &category)
{
    m_displayCategory = category;
}

void ExternalTool::setDisplayName(const QString &name)
{
    m_displayName = name;
}

void ExternalTool::setDescription(const QString &description)
{
    m_description = description;
}


void ExternalTool::setOutputHandling(OutputHandling handling)
{
    m_outputHandling = handling;
}


void ExternalTool::setErrorHandling(OutputHandling handling)
{
    m_errorHandling = handling;
}


void ExternalTool::setModifiesCurrentDocument(bool modifies)
{
    m_modifiesCurrentDocument = modifies;
}


void ExternalTool::setExecutables(const QStringList &executables)
{
    m_executables = executables;
}


void ExternalTool::setArguments(const QString &arguments)
{
    m_arguments = arguments;
}


void ExternalTool::setInput(const QString &input)
{
    m_input = input;
}


void ExternalTool::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

static QStringList splitLocale(const QString &locale)
{
    QString value = locale;
    QStringList values;
    if (!value.isEmpty())
        values << value;
    int index = value.indexOf(QLatin1Char('.'));
    if (index >= 0) {
        value = value.left(index);
        if (!value.isEmpty())
            values << value;
    }
    index = value.indexOf(QLatin1Char('_'));
    if (index >= 0) {
        value = value.left(index);
        if (!value.isEmpty())
            values << value;
    }
    return values;
}

static void localizedText(const QStringList &locales, QXmlStreamReader *reader, int *currentLocale, QString *currentText)
{
    Q_ASSERT(reader);
    Q_ASSERT(currentLocale);
    Q_ASSERT(currentText);
    if (reader->attributes().hasAttribute(QLatin1String(kXmlLang))) {
        int index = locales.indexOf(reader->attributes().value(QLatin1String(kXmlLang)).toString());
        if (index >= 0 && (index < *currentLocale || *currentLocale < 0)) {
            *currentText = reader->readElementText();
            *currentLocale = index;
        } else {
            reader->skipCurrentElement();
        }
    } else {
        if (*currentLocale < 0 && currentText->isEmpty()) {
            *currentText = reader->readElementText();
        } else {
            reader->skipCurrentElement();
        }
    }
    if (currentText->isNull()) // prefer isEmpty over isNull
        *currentText = QLatin1String("");
}

static bool parseOutputAttribute(const QString &attribute, QXmlStreamReader *reader, ExternalTool::OutputHandling *value)
{
    const QString output = reader->attributes().value(attribute).toString();
    if (output == QLatin1String(kOutputShowInPane)) {
        *value = ExternalTool::ShowInPane;
    } else if (output == QLatin1String(kOutputReplaceSelection)) {
        *value = ExternalTool::ReplaceSelection;
    } else if (output == QLatin1String(kOutputIgnore)) {
        *value = ExternalTool::Ignore;
    } else {
        reader->raiseError(QLatin1String("Allowed values for output attribute are 'showinpane','replaceselection','ignore'"));
        return false;
    }
    return true;
}

ExternalTool * ExternalTool::createFromXml(const QByteArray &xml, QString *errorMessage, const QString &locale)
{
    int descriptionLocale = -1;
    int nameLocale = -1;
    int categoryLocale = -1;
    const QStringList &locales = splitLocale(locale);
    ExternalTool *tool = new ExternalTool;
    QXmlStreamReader reader(xml);

    if (!reader.readNextStartElement() || reader.name() != QLatin1String(kExternalTool))
        reader.raiseError(QLatin1String("Missing start element <externaltool>"));
    tool->m_id = reader.attributes().value(QLatin1String(kId)).toString();
    if (tool->m_id.isEmpty())
        reader.raiseError(QLatin1String("Missing or empty id attribute for <externaltool>"));
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String(kDescription)) {
            localizedText(locales, &reader, &descriptionLocale, &tool->m_description);
        } else if (reader.name() == QLatin1String(kDisplayName)) {
            localizedText(locales, &reader, &nameLocale, &tool->m_displayName);
        } else if (reader.name() == QLatin1String(kCategory)) {
            localizedText(locales, &reader, &categoryLocale, &tool->m_displayCategory);
        } else if (reader.name() == QLatin1String(kOrder)) {
            if (tool->m_order >= 0) {
                reader.raiseError(QLatin1String("only one <order> element allowed"));
                break;
            }
            bool ok;
            tool->m_order = reader.readElementText().toInt(&ok);
            if (!ok || tool->m_order < 0)
                reader.raiseError(QLatin1String("<order> element requires non-negative integer value"));
        } else if (reader.name() == QLatin1String(kExecutable)) {
            if (reader.attributes().hasAttribute(QLatin1String(kOutput))) {
                if (!parseOutputAttribute(QLatin1String(kOutput), &reader, &tool->m_outputHandling))
                    break;
            }
            if (reader.attributes().hasAttribute(QLatin1String(kError))) {
                if (!parseOutputAttribute(QLatin1String(kError), &reader, &tool->m_errorHandling))
                    break;
            }
            if (reader.attributes().hasAttribute(QLatin1String(kModifiesDocument))) {
                const QString &value = reader.attributes().value(QLatin1String(kModifiesDocument)).toString();
                if (value == QLatin1String(kYes) || value == QLatin1String(kTrue)) {
                    tool->m_modifiesCurrentDocument = true;
                } else if (value == QLatin1String(kNo) || value == QLatin1String(kFalse)) {
                    tool->m_modifiesCurrentDocument = false;
                } else {
                    reader.raiseError(QLatin1String("Allowed values for modifiesdocument attribute are 'yes','true','no','false'"));
                    break;
                }
            }
            while (reader.readNextStartElement()) {
                if (reader.name() == QLatin1String(kPath)) {
                    tool->m_executables.append(reader.readElementText());
                } else if (reader.name() == QLatin1String(kArguments)) {
                    if (!tool->m_arguments.isEmpty()) {
                        reader.raiseError(QLatin1String("only one <arguments> element allowed"));
                        break;
                    }
                    tool->m_arguments = reader.readElementText();
                } else if (reader.name() == QLatin1String(kInput)) {
                    if (!tool->m_input.isEmpty()) {
                        reader.raiseError(QLatin1String("only one <input> element allowed"));
                        break;
                    }
                    tool->m_input = reader.readElementText();
                } else if (reader.name() == QLatin1String(kWorkingDirectory)) {
                    if (!tool->m_workingDirectory.isEmpty()) {
                        reader.raiseError(QLatin1String("only one <workingdirectory> element allowed"));
                        break;
                    }
                    tool->m_workingDirectory = reader.readElementText();
                } else {
                    reader.raiseError(QString::fromLatin1("Unknown element <%1> as subelement of <%2>").arg(
                                          reader.qualifiedName().toString(), QLatin1String(kExecutable)));
                    break;
                }
            }
        } else {
            reader.raiseError(QString::fromLatin1("Unknown element <%1>").arg(reader.qualifiedName().toString()));
        }
    }
    if (reader.hasError()) {
        if (errorMessage)
            *errorMessage = reader.errorString();
        delete tool;
        return 0;
    }
    return tool;
}

ExternalTool * ExternalTool::createFromFile(const QString &fileName, QString *errorMessage, const QString &locale)
{
    QFileInfo info(fileName);
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = tr("Could not open tool specification %1 for reading: %2").
                            arg(fileName, file.errorString());
        return 0;
    }
    const QByteArray &bytes = file.readAll();
    file.close();
    ExternalTool *tool = ExternalTool::createFromXml(bytes, errorMessage, locale);
    if (!tool) {
        return 0;
    }
    tool->m_fileName = file.fileName();
    return tool;
}

static QLatin1String stringForOutputHandling(ExternalTool::OutputHandling handling)
{
    switch (handling) {
    case Core::Internal::ExternalTool::Ignore:
        return QLatin1String(kOutputIgnore);
    case Core::Internal::ExternalTool::ShowInPane:
        return QLatin1String(kOutputShowInPane);
    case Core::Internal::ExternalTool::ReplaceSelection:
        return QLatin1String(kOutputReplaceSelection);
    }
    return QLatin1String("");
}

bool ExternalTool::save(QString *errorMessage) const
{
    if (m_fileName.isEmpty())
        return false;
    QFile file(m_fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = tr("Could not write tool specification %1: %2").
                            arg(m_fileName, file.errorString());
        return false;
    }
    QXmlStreamWriter out(&file);
    out.setAutoFormatting(true);
    out.writeStartDocument(QLatin1String("1.0"));
    out.writeComment(QString::fromLatin1("Written on %1 by Qt Creator %2")
                     .arg(QDateTime::currentDateTime().toString(), QLatin1String(Constants::IDE_VERSION_LONG)));
    out.writeStartElement(QLatin1String(kExternalTool));
    out.writeAttribute(QLatin1String(kId), m_id);
    out.writeTextElement(QLatin1String(kDescription), m_description);
    out.writeTextElement(QLatin1String(kDisplayName), m_displayName);
    out.writeTextElement(QLatin1String(kCategory), m_displayCategory);
    if (m_order != -1)
        out.writeTextElement(QLatin1String(kOrder), QString::number(m_order));

    out.writeStartElement(QLatin1String(kExecutable));
    out.writeAttribute(QLatin1String(kOutput), stringForOutputHandling(m_outputHandling));
    out.writeAttribute(QLatin1String(kError), stringForOutputHandling(m_errorHandling));
    out.writeAttribute(QLatin1String(kModifiesDocument), m_modifiesCurrentDocument ? QLatin1String(kYes) : QLatin1String(kNo));
    foreach (const QString &executable, m_executables)
        out.writeTextElement(QLatin1String(kPath), executable);
    if (!m_arguments.isEmpty())
        out.writeTextElement(QLatin1String(kArguments), m_arguments);
    if (!m_input.isEmpty())
        out.writeTextElement(QLatin1String(kInput), m_input);
    if (!m_workingDirectory.isEmpty())
        out.writeTextElement(QLatin1String(kWorkingDirectory), m_workingDirectory);
    out.writeEndElement();

    out.writeEndDocument();
    file.close();
    return true;
}

bool ExternalTool::operator==(const ExternalTool &other) const
{
    return m_id == other.m_id
            && m_description == other.m_description
            && m_displayName == other.m_displayName
            && m_displayCategory == other.m_displayCategory
            && m_order == other.m_order
            && m_executables == other.m_executables
            && m_arguments == other.m_arguments
            && m_input == other.m_input
            && m_workingDirectory == other.m_workingDirectory
            && m_outputHandling == other.m_outputHandling
            && m_modifiesCurrentDocument == other.m_modifiesCurrentDocument
            && m_errorHandling == other.m_errorHandling
            && m_fileName == other.m_fileName;
}

// #pragma mark -- ExternalToolRunner

ExternalToolRunner::ExternalToolRunner(const ExternalTool *tool)
    : m_tool(new ExternalTool(tool)),
      m_process(0),
      m_outputCodec(QTextCodec::codecForLocale()),
      m_hasError(false)
{
    run();
}

ExternalToolRunner::~ExternalToolRunner()
{
    if (m_tool)
        delete m_tool;
}

bool ExternalToolRunner::hasError() const
{
    return m_hasError;
}

QString ExternalToolRunner::errorString() const
{
    return m_errorString;
}

bool ExternalToolRunner::resolve()
{
    if (!m_tool)
        return false;
    m_resolvedExecutable.clear();
    m_resolvedArguments.clear();
    m_resolvedWorkingDirectory.clear();
    { // executable
        QStringList expandedExecutables; /* for error message */
        foreach (const QString &executable, m_tool->executables()) {
            QString expanded = Utils::expandMacros(executable,
                                                   Core::VariableManager::instance()->macroExpander());
            expandedExecutables << expanded;
            m_resolvedExecutable =
                    Utils::Environment::systemEnvironment().searchInPath(expanded);
            if (!m_resolvedExecutable.isEmpty())
                break;
        }
        if (m_resolvedExecutable.isEmpty()) {
            m_hasError = true;
            for (int i = 0; i < expandedExecutables.size(); ++i) {
                m_errorString += tr("Could not find executable for '%1' (expanded '%2')\n")
                        .arg(m_tool->executables().at(i))
                        .arg(expandedExecutables.at(i));
            }
            if (!m_errorString.isEmpty())
                m_errorString.chop(1);
            return false;
        }
    }
    { // arguments
        m_resolvedArguments = Utils::QtcProcess::expandMacros(m_tool->arguments(),
                                               Core::VariableManager::instance()->macroExpander());
    }
    { // input
        m_resolvedInput = Utils::expandMacros(m_tool->input(),
                                              Core::VariableManager::instance()->macroExpander());
    }
    { // working directory
        m_resolvedWorkingDirectory = Utils::expandMacros(m_tool->workingDirectory(),
                                               Core::VariableManager::instance()->macroExpander());
    }
    return true;
}

void ExternalToolRunner::run()
{
    if (!resolve()) {
        deleteLater();
        return;
    }
    if (m_tool->modifiesCurrentDocument()) {
        if (IEditor *editor = EditorManager::instance()->currentEditor()) {
            m_expectedFileName = editor->file()->fileName();
            bool cancelled = false;
            FileManager::instance()->saveModifiedFiles(QList<IFile *>() << editor->file(), &cancelled);
            if (cancelled) {
                deleteLater();
                return;
            }
            FileManager::instance()->expectFileChange(m_expectedFileName);
        }
    }
    m_process = new Utils::QtcProcess(this);
    connect(m_process, SIGNAL(started()), this, SLOT(started()));
    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int,QProcess::ExitStatus)));
    connect(m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
    if (!m_resolvedWorkingDirectory.isEmpty())
        m_process->setWorkingDirectory(m_resolvedWorkingDirectory);
    m_process->setCommand(m_resolvedExecutable, m_resolvedArguments);
    ICore::instance()->messageManager()->printToOutputPane(
                tr("Starting external tool '%1' %2").arg(m_resolvedExecutable, m_resolvedArguments), false);
    m_process->start();
}

void ExternalToolRunner::started()
{
    if (!m_resolvedInput.isEmpty()) {
        m_process->write(m_resolvedInput.toLocal8Bit());
    }
    m_process->closeWriteChannel();
}

void ExternalToolRunner::finished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::NormalExit && exitCode == 0) {
        if (m_tool->outputHandling() == ExternalTool::ReplaceSelection
                || m_tool->errorHandling() == ExternalTool::ReplaceSelection) {
            emit ExternalToolManager::instance()->replaceSelectionRequested(m_processOutput);
        }
        if (m_tool->modifiesCurrentDocument()) {
            FileManager::instance()->unexpectFileChange(m_expectedFileName);
        }
    }
    ICore::instance()->messageManager()->printToOutputPane(
                tr("'%1' finished").arg(m_resolvedExecutable), false);
    deleteLater();
}

void ExternalToolRunner::error(QProcess::ProcessError error)
{
    if (m_tool->modifiesCurrentDocument()) {
        FileManager::instance()->unexpectFileChange(m_expectedFileName);
    }
    // TODO inform about errors
    deleteLater();
}

void ExternalToolRunner::readStandardOutput()
{
    if (m_tool->outputHandling() == ExternalTool::Ignore)
        return;
    QByteArray data = m_process->readAllStandardOutput();
    QString output = m_outputCodec->toUnicode(data.constData(), data.length(), &m_outputCodecState);
    if (m_tool->outputHandling() == ExternalTool::ShowInPane) {
        ICore::instance()->messageManager()->printToOutputPane(output, true);
    } else if (m_tool->outputHandling() == ExternalTool::ReplaceSelection) {
        m_processOutput.append(output);
    }
}

void ExternalToolRunner::readStandardError()
{
    if (m_tool->errorHandling() == ExternalTool::Ignore)
        return;
    QByteArray data = m_process->readAllStandardError();
    QString output = m_outputCodec->toUnicode(data.constData(), data.length(), &m_errorCodecState);
    if (m_tool->errorHandling() == ExternalTool::ShowInPane) {
        ICore::instance()->messageManager()->printToOutputPane(output, true);
    } else if (m_tool->errorHandling() == ExternalTool::ReplaceSelection) {
        m_processOutput.append(output);
    }
}

// #pragma mark -- ExternalToolManager

ExternalToolManager *ExternalToolManager::m_instance = 0;

ExternalToolManager::ExternalToolManager(Core::ICore *core)
    : QObject(core), m_core(core)
{
    m_instance = this;
    initialize();
}

ExternalToolManager::~ExternalToolManager()
{
    writeSettings();
    // TODO kill running tools
    qDeleteAll(m_tools);
}

void ExternalToolManager::initialize()
{
    m_configureSeparator = new QAction(this);
    m_configureSeparator->setSeparator(true);
    m_configureAction = new QAction(tr("Configure..."), this);
    connect(m_configureAction, SIGNAL(triggered()), this, SLOT(openPreferences()));

    // add the external tools menu
    ActionManager *am = m_core->actionManager();
    ActionContainer *mexternaltools = am->createMenu(Id(Constants::M_TOOLS_EXTERNAL));
    mexternaltools->menu()->setTitle(tr("External"));
    ActionContainer *mtools = am->actionContainer(Constants::M_TOOLS);
    mtools->addMenu(mexternaltools, Constants::G_DEFAULT_THREE);

    QMap<QString, QMultiMap<int, ExternalTool*> > categoryPriorityMap;
    QMap<QString, ExternalTool *> tools;
    parseDirectory(m_core->userResourcePath() + QLatin1String("/externaltools"),
                   &categoryPriorityMap,
                   &tools);
    parseDirectory(m_core->resourcePath() + QLatin1String("/externaltools"),
                   &categoryPriorityMap,
                   &tools,
                   true);

    QMap<QString, QList<Internal::ExternalTool *> > categoryMap;
    QMapIterator<QString, QMultiMap<int, ExternalTool*> > it(categoryPriorityMap);
    while (it.hasNext()) {
        it.next();
        categoryMap.insert(it.key(), it.value().values());
    }

    // read renamed categories and custom order
    readSettings(tools, &categoryMap);
    setToolsByCategory(categoryMap);
}

void ExternalToolManager::parseDirectory(const QString &directory,
                                         QMap<QString, QMultiMap<int, Internal::ExternalTool*> > *categoryMenus,
                                         QMap<QString, ExternalTool *> *tools,
                                         bool isPreset)
{
    QTC_ASSERT(categoryMenus, return);
    QTC_ASSERT(tools, return);
    QDir dir(directory, QLatin1String("*.xml"), QDir::Unsorted, QDir::Files | QDir::Readable);
    foreach (const QFileInfo &info, dir.entryInfoList()) {
        const QString &fileName = info.absoluteFilePath();
        QString error;
        ExternalTool *tool = ExternalTool::createFromFile(fileName, &error, m_core->userInterfaceLanguage());
        if (!tool) {
            qWarning() << tr("Error while parsing external tool %1: %2").arg(fileName, error);
            continue;
        }
        if (tools->contains(tool->id())) {
            if (isPreset) {
                // preset that was changed
                ExternalTool *other = tools->value(tool->id());
                other->setPreset(QSharedPointer<ExternalTool>(tool));
            } else {
                qWarning() << tr("Error: External tool in %1 has duplicate id").arg(fileName);
                delete tool;
            }
            continue;
        }
        if (isPreset) {
            // preset that wasn't changed --> save original values
            tool->setPreset(QSharedPointer<ExternalTool>(new ExternalTool(tool)));
        }
        tools->insert(tool->id(), tool);
        (*categoryMenus)[tool->displayCategory()].insert(tool->order(), tool);
    }
}

void ExternalToolManager::menuActivated()
{
    QAction *action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return);
    ExternalTool *tool = m_tools.value(action->data().toString());
    QTC_ASSERT(tool, return);
    ExternalToolRunner *runner = new ExternalToolRunner(tool);
    if (runner->hasError()) {
        ICore::instance()->messageManager()->printToOutputPane(runner->errorString(), true);
    }
}

QMap<QString, QList<Internal::ExternalTool *> > ExternalToolManager::toolsByCategory() const
{
    return m_categoryMap;
}

QMap<QString, ExternalTool *> ExternalToolManager::toolsById() const
{
    return m_tools;
}

void ExternalToolManager::setToolsByCategory(const QMap<QString, QList<Internal::ExternalTool *> > &tools)
{
    // clear menu
    ActionManager *am = m_core->actionManager();
    ActionContainer *mexternaltools = am->actionContainer(Id(Constants::M_TOOLS_EXTERNAL));
    mexternaltools->clear();

    // delete old tools and create list of new ones
    QMap<QString, ExternalTool *> newTools;
    QMap<QString, QAction *> newActions;
    QMapIterator<QString, QList<ExternalTool *> > it(tools);
    while (it.hasNext()) {
        it.next();
        foreach (ExternalTool *tool, it.value()) {
            const QString &id = tool->id();
            if (m_tools.value(id) == tool) {
                newActions.insert(id, m_actions.value(id));
                // remove from list to prevent deletion
                m_tools.remove(id);
                m_actions.remove(id);
            }
            newTools.insert(id, tool);
        }
    }
    qDeleteAll(m_tools);
    QMapIterator<QString, QAction *> remainingActions(m_actions);
    while (remainingActions.hasNext()) {
        remainingActions.next();
        am->unregisterAction(remainingActions.value(), Id("Tools.External." + remainingActions.key()));
        delete remainingActions.value();
    }
    m_actions.clear();
    // assign the new stuff
    m_tools = newTools;
    m_actions = newActions;
    m_categoryMap = tools;
    // create menu structure and remove no-longer used containers
    // add all the category menus, QMap is nicely sorted
    QMap<QString, ActionContainer *> newContainers;
    it.toFront();
    while (it.hasNext()) {
        it.next();
        ActionContainer *container = 0;
        const QString &containerName = it.key();
        if (containerName.isEmpty()) { // no displayCategory, so put into external tools menu directly
            container = mexternaltools;
        } else {
            if (m_containers.contains(containerName)) {
                container = m_containers.take(containerName); // remove to avoid deletion below
            } else {
                container = am->createMenu(Id("Tools.External.Category." + containerName));
            }
            newContainers.insert(containerName, container);
            mexternaltools->addMenu(container, Constants::G_DEFAULT_ONE);
            container->menu()->setTitle(containerName);
        }
        foreach (ExternalTool *tool, it.value()) {
            const QString &toolId = tool->id();
            // tool action and command
            QAction *action = 0;
            Command *command = 0;
            if (m_actions.contains(toolId)) {
                action = m_actions.value(toolId);
                command = am->command(Id("Tools.External." + toolId));
            } else {
                action = new QAction(tool->displayName(), this);
                action->setData(toolId);
                m_actions.insert(toolId, action);
                connect(action, SIGNAL(triggered()), this, SLOT(menuActivated()));
                command = am->registerAction(action, Id("Tools.External." + toolId), Context(Constants::C_GLOBAL));
                command->setAttribute(Command::CA_UpdateText);
            }
            action->setText(tool->displayName());
            action->setToolTip(tool->description());
            action->setWhatsThis(tool->description());
            container->addAction(command, Constants::G_DEFAULT_TWO);
        }
    }

    // delete the unused containers
    qDeleteAll(m_containers);
    // remember the new containers
    m_containers = newContainers;

    // (re)add the configure menu item
    mexternaltools->menu()->addAction(m_configureSeparator);
    mexternaltools->menu()->addAction(m_configureAction);
}

void ExternalToolManager::readSettings(const QMap<QString, ExternalTool *> &tools,
                                       QMap<QString, QList<ExternalTool *> > *categoryMap)
{
    QSettings *settings = m_core->settings();
    settings->beginGroup(QLatin1String("ExternalTools"));

    if (categoryMap) {
        settings->beginGroup(QLatin1String("OverrideCategories"));
        foreach (const QString &settingsCategory, settings->childGroups()) {
            QString displayCategory = settingsCategory;
            if (displayCategory == QLatin1String(kSpecialUncategorizedSetting))
                displayCategory = QLatin1String("");
            int count = settings->beginReadArray(settingsCategory);
            for (int i = 0; i < count; ++i) {
                settings->setArrayIndex(i);
                const QString &toolId = settings->value(QLatin1String("Tool")).toString();
                if (tools.contains(toolId)) {
                    ExternalTool *tool = tools.value(toolId);
                    // remove from old category
                    (*categoryMap)[tool->displayCategory()].removeAll(tool);
                    if (categoryMap->value(tool->displayCategory()).isEmpty())
                        categoryMap->remove(tool->displayCategory());
                    // add to new category
                    (*categoryMap)[displayCategory].append(tool);
                }
            }
            settings->endArray();
        }
        settings->endGroup();
    }

    settings->endGroup();
}

void ExternalToolManager::writeSettings()
{
    QSettings *settings = m_core->settings();
    settings->beginGroup(QLatin1String("ExternalTools"));
    settings->remove(QLatin1String(""));

    settings->beginGroup(QLatin1String("OverrideCategories"));
    QMapIterator<QString, QList<ExternalTool *> > it(m_categoryMap);
    while (it.hasNext()) {
        it.next();
        QString category = it.key();
        if (category.isEmpty())
            category = QLatin1String(kSpecialUncategorizedSetting);
        settings->beginWriteArray(category, it.value().count());
        int i = 0;
        foreach (ExternalTool *tool, it.value()) {
            settings->setArrayIndex(i);
            settings->setValue(QLatin1String("Tool"), tool->id());
            ++i;
        }
        settings->endArray();
    }
    settings->endGroup();

    settings->endGroup();
}

void ExternalToolManager::openPreferences()
{
    ICore::instance()->showOptionsDialog(QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE),
                                         QLatin1String(Core::Constants::SETTINGS_ID_TOOLS));
}
