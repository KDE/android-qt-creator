/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "basefilewizard.h"

#include "coreconstants.h"
#include "icore.h"
#include "ifilewizardextension.h"
#include "mimedatabase.h"
#include "editormanager/editormanager.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/filewizarddialog.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QVector>
#include <QtCore/QDebug>
#include <QtCore/QSharedData>
#include <QtCore/QEventLoop>
#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>

#include <QtGui/QMessageBox>
#include <QtGui/QWizard>
#include <QtGui/QMainWindow>
#include <QtGui/QIcon>

enum { debugWizard = 0 };

namespace Core {

class GeneratedFilePrivate : public QSharedData
{
public:
    GeneratedFilePrivate() : binary(false) {}
    explicit GeneratedFilePrivate(const QString &p);
    QString path;
    QByteArray contents;
    QString editorId;
    bool binary;
    GeneratedFile::Attributes attributes;
};

GeneratedFilePrivate::GeneratedFilePrivate(const QString &p) :
    path(QDir::cleanPath(p)),
    binary(false),
    attributes(0)
{
}

GeneratedFile::GeneratedFile() :
    m_d(new GeneratedFilePrivate)
{
}

GeneratedFile::GeneratedFile(const QString &p) :
    m_d(new GeneratedFilePrivate(p))
{
}

GeneratedFile::GeneratedFile(const GeneratedFile &rhs) :
    m_d(rhs.m_d)
{
}

GeneratedFile &GeneratedFile::operator=(const GeneratedFile &rhs)
{
    if (this != &rhs)
        m_d.operator=(rhs.m_d);
    return *this;
}

GeneratedFile::~GeneratedFile()
{
}

QString GeneratedFile::path() const
{
    return m_d->path;
}

void GeneratedFile::setPath(const QString &p)
{
    m_d->path = QDir::cleanPath(p);
}

QString GeneratedFile::contents() const
{
    return QString::fromUtf8(m_d->contents);
}

void GeneratedFile::setContents(const QString &c)
{
    m_d->contents = c.toUtf8();
}

QByteArray GeneratedFile::binaryContents() const
{
    return m_d->contents;
}

void GeneratedFile::setBinaryContents(const QByteArray &c)
{
    m_d->contents = c;
}

bool GeneratedFile::isBinary() const
{
    return m_d->binary;
}

void GeneratedFile::setBinary(bool b)
{
    m_d->binary = b;
}

QString GeneratedFile::editorId() const
{
    return m_d->editorId;
}

void GeneratedFile::setEditorId(const QString &k)
{
    m_d->editorId = k;
}

bool GeneratedFile::write(QString *errorMessage) const
{
    // Ensure the directory
    const QFileInfo info(m_d->path);
    const QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            *errorMessage = BaseFileWizard::tr("Unable to create the directory %1.").arg(dir.absolutePath());
            return false;
        }
    }
    // Write out
    QFile file(m_d->path);

    QIODevice::OpenMode flags = QIODevice::WriteOnly|QIODevice::Truncate;
    if (!isBinary())
        flags |= QIODevice::Text;

    if (!file.open(flags)) {
        *errorMessage = BaseFileWizard::tr("Unable to open %1 for writing: %2").arg(m_d->path, file.errorString());
        return false;
    }
    if (file.write(m_d->contents) == -1) {
        *errorMessage =  BaseFileWizard::tr("Error while writing to %1: %2").arg(m_d->path, file.errorString());
        return false;
    }
    file.close();
    return true;
}

GeneratedFile::Attributes GeneratedFile::attributes() const
{
    return m_d->attributes;
}

void GeneratedFile::setAttributes(Attributes a)
{
    m_d->attributes = a;
}

// ------------ BaseFileWizardParameterData
class BaseFileWizardParameterData : public QSharedData
{
public:
    explicit BaseFileWizardParameterData(IWizard::WizardKind kind = IWizard::FileWizard);
    void clear();

    IWizard::WizardKind kind;
    QIcon icon;
    QString description;
    QString displayName;
    QString id;
    QString category;
    QString displayCategory;
};

BaseFileWizardParameterData::BaseFileWizardParameterData(IWizard::WizardKind k) :
    kind(k)
{
}

void BaseFileWizardParameterData::clear()
{
    kind = IWizard::FileWizard;
    icon = QIcon();
    description.clear();
    displayName.clear();
    id.clear();
    category.clear();
    displayCategory.clear();
}

BaseFileWizardParameters::BaseFileWizardParameters(IWizard::WizardKind kind) :
   m_d(new BaseFileWizardParameterData(kind))
{
}

BaseFileWizardParameters::BaseFileWizardParameters(const BaseFileWizardParameters &rhs) :
    m_d(rhs.m_d)
{
}

BaseFileWizardParameters &BaseFileWizardParameters::operator=(const BaseFileWizardParameters &rhs)
{
    if (this != &rhs)
        m_d.operator=(rhs.m_d);
    return *this;
}

BaseFileWizardParameters::~BaseFileWizardParameters()
{
}

void BaseFileWizardParameters::clear()
{
    m_d->clear();
}

CORE_EXPORT QDebug operator<<(QDebug d, const BaseFileWizardParameters &p)
{
    d.nospace() << "Kind: " << p.kind() << " Id: " << p.id()
                << " Category: " << p.category()
                << " DisplayName: " << p.displayName()
                << " Description: " << p.description()
                << " DisplayCategory: " << p.displayCategory();
    return d;
}

IWizard::WizardKind BaseFileWizardParameters::kind() const
{
    return m_d->kind;
}

void BaseFileWizardParameters::setKind(IWizard::WizardKind k)
{
    m_d->kind = k;
}

QIcon BaseFileWizardParameters::icon() const
{
    return m_d->icon;
}

void BaseFileWizardParameters::setIcon(const QIcon &icon)
{
    m_d->icon = icon;
}

QString BaseFileWizardParameters::description() const
{
    return m_d->description;
}

void BaseFileWizardParameters::setDescription(const QString &v)
{
    m_d->description = v;
}

QString BaseFileWizardParameters::displayName() const
{
    return m_d->displayName;
}

void BaseFileWizardParameters::setDisplayName(const QString &v)
{
    m_d->displayName = v;
}

QString BaseFileWizardParameters::id() const
{
    return m_d->id;
}

void BaseFileWizardParameters::setId(const QString &v)
{
    m_d->id = v;
}

QString BaseFileWizardParameters::category() const
{
    return m_d->category;
}

void BaseFileWizardParameters::setCategory(const QString &v)
{
    m_d->category = v;
}

QString BaseFileWizardParameters::displayCategory() const
{
    return m_d->displayCategory;
}

void BaseFileWizardParameters::setDisplayCategory(const QString &v)
{
    m_d->displayCategory = v;
}

/* WizardEventLoop: Special event loop that runs a QWizard and terminates if the page changes.
 * Synopsis:
 * \code
    Wizard wizard(parent);
    WizardEventLoop::WizardResult wr;
    do {
        wr = WizardEventLoop::execWizardPage(wizard);
    } while (wr == WizardEventLoop::PageChanged);
 * \endcode */

class WizardEventLoop : public QEventLoop
{
    Q_OBJECT
    Q_DISABLE_COPY(WizardEventLoop)
    WizardEventLoop(QObject *parent);

public:
    enum WizardResult { Accepted, Rejected , PageChanged };

    static WizardResult execWizardPage(QWizard &w);

private slots:
    void pageChanged(int);
    void accepted();
    void rejected();

private:
    WizardResult execWizardPageI();

    WizardResult m_result;
};

WizardEventLoop::WizardEventLoop(QObject *parent) :
    QEventLoop(parent),
    m_result(Rejected)
{
}

WizardEventLoop::WizardResult WizardEventLoop::execWizardPage(QWizard &wizard)
{
    /* Install ourselves on the wizard. Main trick is here to connect
     * to the page changed signal and quit() on it. */
    WizardEventLoop *eventLoop = wizard.findChild<WizardEventLoop *>();
    if (!eventLoop) {
        eventLoop = new WizardEventLoop(&wizard);
        connect(&wizard, SIGNAL(currentIdChanged(int)), eventLoop, SLOT(pageChanged(int)));
        connect(&wizard, SIGNAL(accepted()), eventLoop, SLOT(accepted()));
        connect(&wizard, SIGNAL(rejected()), eventLoop, SLOT(rejected()));
        wizard.setAttribute(Qt::WA_ShowModal, true);
        wizard.show();
    }
    const WizardResult result = eventLoop->execWizardPageI();
    // Quitting?
    if (result != PageChanged)
        delete eventLoop;
    if (debugWizard)
        qDebug() << "WizardEventLoop::runWizard" << wizard.pageIds() << " returns " << result;

    return result;
}

WizardEventLoop::WizardResult WizardEventLoop::execWizardPageI()
{
    m_result = Rejected;
    exec(QEventLoop::DialogExec);
    return m_result;
}

void WizardEventLoop::pageChanged(int /*page*/)
{
    m_result = PageChanged;
    quit(); // !
}

void WizardEventLoop::accepted()
{
    m_result = Accepted;
    quit();
}

void WizardEventLoop::rejected()
{
    m_result = Rejected;
    quit();
}

// ---------------- BaseFileWizardPrivate
struct BaseFileWizardPrivate
{
    explicit BaseFileWizardPrivate(const Core::BaseFileWizardParameters &parameters)
      : m_parameters(parameters), m_wizardDialog(0)
    {}

    const Core::BaseFileWizardParameters m_parameters;
    QWizard *m_wizardDialog;
};

// ---------------- Wizard
BaseFileWizard::BaseFileWizard(const BaseFileWizardParameters &parameters,
                       QObject *parent) :
    IWizard(parent),
    m_d(new BaseFileWizardPrivate(parameters))
{
}

BaseFileWizard::~BaseFileWizard()
{
    delete m_d;
}

IWizard::WizardKind  BaseFileWizard::kind() const
{
    return m_d->m_parameters.kind();
}

QIcon BaseFileWizard::icon() const
{
    return m_d->m_parameters.icon();
}

QString BaseFileWizard::description() const
{
    return m_d->m_parameters.description();
}

QString BaseFileWizard::displayName() const
{
    return m_d->m_parameters.displayName();
}

QString BaseFileWizard::id() const
{
    return m_d->m_parameters.id();
}

QString BaseFileWizard::category() const
{
    return m_d->m_parameters.category();
}

QString BaseFileWizard::displayCategory() const
{
    return m_d->m_parameters.displayCategory();
}

void BaseFileWizard::runWizard(const QString &path, QWidget *parent)
{
    QTC_ASSERT(!path.isEmpty(), return);

    typedef  QList<IFileWizardExtension*> ExtensionList;

    QString errorMessage;
    // Compile extension pages, purge out unused ones
    ExtensionList extensions = ExtensionSystem::PluginManager::instance()->getObjects<IFileWizardExtension>();
    WizardPageList  allExtensionPages;
    for (ExtensionList::iterator it = extensions.begin(); it !=  extensions.end(); ) {
        const WizardPageList extensionPages = (*it)->extensionPages(this);
        if (extensionPages.empty()) {
            it = extensions.erase(it);
        } else {
            allExtensionPages += extensionPages;
            ++it;
        }
    }

    if (debugWizard)
        qDebug() << Q_FUNC_INFO <<  path << parent << "exs" <<  extensions.size() << allExtensionPages.size();

    QWizardPage *firstExtensionPage = 0;
    if (!allExtensionPages.empty())
        firstExtensionPage = allExtensionPages.front();

    // Create dialog and run it. Ensure that the dialog is deleted when
    // leaving the func, but not before the IFileWizardExtension::process
    // has been called
    const QScopedPointer<QWizard> wizard(createWizardDialog(parent, path, allExtensionPages));
    QTC_ASSERT(!wizard.isNull(), return);

    GeneratedFiles files;
    // Run the wizard: Call generate files on switching to the first extension
    // page is OR after 'Accepted' if there are no extension pages
    while (true) {
        const WizardEventLoop::WizardResult wr = WizardEventLoop::execWizardPage(*wizard);
        if (wr == WizardEventLoop::Rejected) {
            files.clear();
            break;
        }
        const bool accepted = wr == WizardEventLoop::Accepted;
        const bool firstExtensionPageHit = wr == WizardEventLoop::PageChanged
                                           && wizard->page(wizard->currentId()) == firstExtensionPage;
        const bool needGenerateFiles = firstExtensionPageHit || (accepted && allExtensionPages.empty());
        if (needGenerateFiles) {
            QString errorMessage;
            files = generateFiles(wizard.data(), &errorMessage);
            if (files.empty()) {
                QMessageBox::critical(0, tr("File Generation Failure"), errorMessage);
                break;
            }
        }
        if (firstExtensionPageHit)
            foreach (IFileWizardExtension *ex, extensions)
                ex->firstExtensionPageShown(files);
        if (accepted)
            break;
    }
    if (files.empty())
        return;
    // Compile result list and prompt for overwrite
    QStringList result;
    foreach (const GeneratedFile &generatedFile, files)
        result.push_back(generatedFile.path());

    switch (promptOverwrite(result, &errorMessage)) {
    case OverwriteCanceled:
        return;
    case OverwriteError:
        QMessageBox::critical(0, tr("Existing files"), errorMessage);
        return;
    case OverwriteOk:
        break;
    }

    // Write
    if (!writeFiles(files, &errorMessage)) {
        QMessageBox::critical(parent, tr("File Generation Failure"), errorMessage);
        return;
    }

    bool removeOpenProjectAttribute = false;
    // Run the extensions
    foreach (IFileWizardExtension *ex, extensions) {
        bool remove;
        if (!ex->process(files, &remove, &errorMessage)) {
            QMessageBox::critical(parent, tr("File Generation Failure"), errorMessage);
            return;
        }
        removeOpenProjectAttribute |= remove;
    }

    if (removeOpenProjectAttribute) {
        for (int i = 0; i < files.count(); i++) {
            if (files[i].attributes() & Core::GeneratedFile::OpenProjectAttribute)
                files[i].setAttributes(Core::GeneratedFile::OpenEditorAttribute);
        }
    }

    // Post generation handler
    if (!postGenerateFiles(wizard.data(), files, &errorMessage))
        QMessageBox::critical(0, tr("File Generation Failure"), errorMessage);
}

// Write
bool BaseFileWizard::writeFiles(const GeneratedFiles &files, QString *errorMessage)
{
    foreach (const GeneratedFile &generatedFile, files)
        if (!(generatedFile.attributes() & GeneratedFile::CustomGeneratorAttribute))
            if (!generatedFile.write(errorMessage))
                return false;
    return true;
}


void BaseFileWizard::setupWizard(QWizard *w)
{
    w->setOption(QWizard::NoCancelButton, false);
    w->setOption(QWizard::NoDefaultButton, false);
    w->setOption(QWizard::NoBackButtonOnStartPage, true);
}

void BaseFileWizard::applyExtensionPageShortTitle(Utils::Wizard *wizard, int pageId)
{
    if (pageId < 0)
        return;
    QWizardPage *p = wizard->page(pageId);
    if (!p)
        return;
    Utils::WizardProgressItem *item = wizard->wizardProgress()->item(pageId);
    if (!item)
        return;
    const QString shortTitle = p->property("shortTitle").toString();
    if (!shortTitle.isEmpty())
      item->setTitle(shortTitle);
}

bool BaseFileWizard::postGenerateFiles(const QWizard *, const GeneratedFiles &l, QString *errorMessage)
{
    return BaseFileWizard::postGenerateOpenEditors(l, errorMessage);
}

bool BaseFileWizard::postGenerateOpenEditors(const GeneratedFiles &l, QString *errorMessage)
{
    Core::EditorManager *em = Core::EditorManager::instance();
    foreach(const Core::GeneratedFile &file, l) {
        if (file.attributes() & Core::GeneratedFile::OpenEditorAttribute) {
            if (!em->openEditor(file.path(), file.editorId(), Core::EditorManager::ModeSwitch )) {
                if (errorMessage)
                    *errorMessage = tr("Failed to open an editor for '%1'.").arg(QDir::toNativeSeparators(file.path()));
                return false;
            }
        }
    }
    return true;
}

BaseFileWizard::OverwriteResult BaseFileWizard::promptOverwrite(const QStringList &files,
                                                                QString *errorMessage) const
{
    if (debugWizard)
        qDebug() << Q_FUNC_INFO << files;

    QStringList existingFiles;
    bool oddStuffFound = false;

    static const QString readOnlyMsg = tr(" [read only]");
    static const QString directoryMsg = tr(" [directory]");
    static const QString symLinkMsg = tr(" [symbolic link]");

    foreach (const QString &fileName, files) {
        const QFileInfo fi(fileName);
        if (fi.exists())
            existingFiles.append(fileName);
    }
    // Note: Generated files are using native separators, no need to convert.
    const QString commonExistingPath = Utils::commonPath(existingFiles);
    // Format a file list message as ( "<file1> [readonly], <file2> [directory]").
    QString fileNamesMsgPart;
    foreach (const QString &fileName, existingFiles) {
        const QFileInfo fi(fileName);
        if (fi.exists()) {
            if (!fileNamesMsgPart.isEmpty())
                fileNamesMsgPart += QLatin1String(", ");
            fileNamesMsgPart += QDir::toNativeSeparators(fileName.mid(commonExistingPath.size() + 1));
            do {
                if (fi.isDir()) {
                    oddStuffFound = true;
                    fileNamesMsgPart += directoryMsg;
                    break;
                }
                if (fi.isSymLink()) {
                    oddStuffFound = true;
                    fileNamesMsgPart += symLinkMsg;
                    break;
            }
                if (!fi.isWritable()) {
                    oddStuffFound = true;
                    fileNamesMsgPart += readOnlyMsg;
                }
            } while (false);
        }
    }

    if (existingFiles.isEmpty())
        return OverwriteOk;

    if (oddStuffFound) {
        *errorMessage = tr("The project directory %1 contains files which cannot be overwritten:\n%2.")
                .arg(QDir::toNativeSeparators(commonExistingPath)).arg(fileNamesMsgPart);
        return OverwriteError;
    }

    const QString messageFormat = tr("The following files already exist in the directory %1:\n"
                                     "%2.\nWould you like to overwrite them?");
    const QString message = messageFormat.arg(QDir::toNativeSeparators(commonExistingPath)).arg(fileNamesMsgPart);
    const bool yes = (QMessageBox::question(Core::ICore::instance()->mainWindow(),
                                            tr("Existing files"), message,
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::No)
                      == QMessageBox::Yes);
    return yes ? OverwriteOk :  OverwriteCanceled;
}

QString BaseFileWizard::buildFileName(const QString &path,
                                      const QString &baseName,
                                      const QString &extension)
{
    QString rc = path;
    if (!rc.isEmpty() && !rc.endsWith(QDir::separator()))
        rc += QDir::separator();
    rc += baseName;
    // Add extension unless user specified something else
    const QChar dot = QLatin1Char('.');
    if (!extension.isEmpty() && !baseName.contains(dot)) {
        if (!extension.startsWith(dot))
            rc += dot;
        rc += extension;
    }
    if (debugWizard)
        qDebug() << Q_FUNC_INFO << rc;
    return rc;
}

QString BaseFileWizard::preferredSuffix(const QString &mimeType)
{
    const QString rc = Core::ICore::instance()->mimeDatabase()->preferredSuffixByType(mimeType);
    if (rc.isEmpty())
        qWarning("%s: WARNING: Unable to find a preferred suffix for %s.",
                 Q_FUNC_INFO, mimeType.toUtf8().constData());
    return rc;
}

// ------------- StandardFileWizard

StandardFileWizard::StandardFileWizard(const BaseFileWizardParameters &parameters,
                                       QObject *parent) :
    BaseFileWizard(parameters, parent)
{
}

QWizard *StandardFileWizard::createWizardDialog(QWidget *parent,
                                                const QString &defaultPath,
                                                const WizardPageList &extensionPages) const
{
    Utils::FileWizardDialog *standardWizardDialog = new Utils::FileWizardDialog(parent);
    standardWizardDialog->setWindowTitle(tr("New %1").arg(displayName()));
    setupWizard(standardWizardDialog);
    standardWizardDialog->setPath(defaultPath);
    foreach (QWizardPage *p, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(standardWizardDialog, standardWizardDialog->addPage(p));
    return standardWizardDialog;
}

GeneratedFiles StandardFileWizard::generateFiles(const QWizard *w,
                                                 QString *errorMessage) const
{
    const Utils::FileWizardDialog *standardWizardDialog = qobject_cast<const Utils::FileWizardDialog *>(w);
    return generateFilesFromPath(standardWizardDialog->path(),
                                 standardWizardDialog->fileName(),
                                 errorMessage);
}

} // namespace Core

#include "basefilewizard.moc"
