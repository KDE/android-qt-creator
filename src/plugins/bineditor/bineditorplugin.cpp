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

#include "bineditorplugin.h"
#include "bineditor.h"
#include "bineditorconstants.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QRegExp>
#include <QtCore/QVariant>

#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QMainWindow>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QRegExpValidator>
#include <QtGui/QToolBar>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/ifile.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>
#include <find/ifindsupport.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>
#include <utils/reloadpromptutils.h>
#include <utils/qtcassert.h>

using namespace BINEditor;
using namespace BINEditor::Internal;


class BinEditorFind : public Find::IFindSupport
{
    Q_OBJECT
public:
    BinEditorFind(BinEditor *editor)
    {
        m_editor = editor;
        m_incrementalStartPos = m_contPos = -1;
    }
    ~BinEditorFind() {}

    bool supportsReplace() const { return false; }
    Find::FindFlags supportedFindFlags() const
    {
        return Find::FindBackward | Find::FindCaseSensitively;
    }

    void resetIncrementalSearch()
    {
        m_incrementalStartPos = m_contPos = -1;
    }

    void clearResults() { m_editor->highlightSearchResults(QByteArray()); }
    QString currentFindString() const { return QString(); }
    QString completedFindString() const { return QString(); }


    int find(const QByteArray &pattern, int pos, Find::FindFlags findFlags) {
        if (pattern.isEmpty()) {
            m_editor->setCursorPosition(pos);
            return pos;
        }

        return m_editor->find(pattern, pos, Find::textDocumentFlagsForFindFlags(findFlags));
    }

    Result findIncremental(const QString &txt, Find::FindFlags findFlags) {
        QByteArray pattern = txt.toLatin1();
        if (pattern != m_lastPattern)
            resetIncrementalSearch(); // Because we don't search for nibbles.
        m_lastPattern = pattern;
        if (m_incrementalStartPos < 0)
            m_incrementalStartPos = m_editor->selectionStart();
        if (m_contPos == -1)
            m_contPos = m_incrementalStartPos;
        int found = find(pattern, m_contPos, findFlags);
        Result result;
        if (found >= 0) {
            result = Found;
            m_editor->highlightSearchResults(pattern, Find::textDocumentFlagsForFindFlags(findFlags));
            m_contPos = -1;
        } else {
            if (found == -2) {
                result = NotYetFound;
                m_contPos +=
                        findFlags & Find::FindBackward
                        ? -BinEditor::SearchStride : BinEditor::SearchStride;
            } else {
                result = NotFound;
                m_contPos = -1;
                m_editor->highlightSearchResults(QByteArray(), 0);
            }
        }
        return result;
    }

    Result findStep(const QString &txt, Find::FindFlags findFlags) {
        QByteArray pattern = txt.toLatin1();
        bool wasReset = (m_incrementalStartPos < 0);
        if (m_contPos == -1) {
            m_contPos = m_editor->cursorPosition();
            if (findFlags & Find::FindBackward)
                m_contPos = m_editor->selectionStart()-1;
        }
        int found = find(pattern, m_contPos, findFlags);
        Result result;
        if (found >= 0) {
            result = Found;
            m_incrementalStartPos = found;
            m_contPos = -1;
            if (wasReset)
                m_editor->highlightSearchResults(pattern, Find::textDocumentFlagsForFindFlags(findFlags));
        } else if (found == -2) {
            result = NotYetFound;
            m_contPos += findFlags & Find::FindBackward
                         ? -BinEditor::SearchStride : BinEditor::SearchStride;
        } else {
            result = NotFound;
            m_contPos = -1;
        }

        return result;
    }

    void replace(const QString &, const QString &,
                 Find::FindFlags) { }
    bool replaceStep(const QString &, const QString &,
                     Find::FindFlags) { return false;}
    int replaceAll(const QString &, const QString &,
                   Find::FindFlags) { return 0; }

private:
    BinEditor *m_editor;
    int m_incrementalStartPos;
    int m_contPos; // Only valid if last result was NotYetFound.
    QByteArray m_lastPattern;
};


class BinEditorFile : public Core::IFile
{
    Q_OBJECT
public:
    BinEditorFile(BinEditor *parent) :
        Core::IFile(parent)
    {
        m_editor = parent;
        connect(m_editor, SIGNAL(dataRequested(Core::IEditor*,quint64)),
            this, SLOT(provideData(Core::IEditor *, quint64)));
        connect(m_editor, SIGNAL(newRangeRequested(Core::IEditor*,quint64)),
            this, SLOT(provideNewRange(Core::IEditor*,quint64)));
        connect(m_editor, SIGNAL(startOfFileRequested(Core::IEditor*)), this,
            SLOT(handleStartOfFileRequested(Core::IEditor*)));
        connect(m_editor, SIGNAL(endOfFileRequested(Core::IEditor*)), this,
            SLOT(handleEndOfFileRequested(Core::IEditor*)));
    }
    ~BinEditorFile() {}

    QString mimeType() const {
        return QLatin1String(Constants::C_BINEDITOR_MIMETYPE);
    }

    bool save(const QString &fileName = QString()) {
        const QString fileNameToUse
            = fileName.isEmpty() ? m_fileName : fileName;
        if (m_editor->save(m_fileName, fileNameToUse)) {
            m_fileName = fileNameToUse;
            m_editor->editor()->setDisplayName(QFileInfo(fileNameToUse).fileName());
            emit changed();
            return true;
        } else {
            return false;
        }
    }

    void rename(const QString &newName) {
        m_fileName = newName;
        m_editor->editor()->setDisplayName(QFileInfo(fileName()).fileName());
        emit changed();
    }

    bool open(const QString &fileName, quint64 offset = 0) {
        QFile file(fileName);
        if (offset < static_cast<quint64>(file.size())
            && file.open(QIODevice::ReadOnly)) {
            m_fileName = fileName;
            m_editor->setSizes(offset, file.size(), true);
            m_editor->editor()->setDisplayName(QFileInfo(fileName).fileName());
            file.close();
            return true;
        }
        return false;
    }

private slots:
    void provideData(Core::IEditor *, quint64 block) {
        if (m_fileName.isEmpty())
            return;
        QFile file(m_fileName);
        if (file.open(QIODevice::ReadOnly)) {
            int blockSize = m_editor->dataBlockSize();
            file.seek(block * blockSize);
            QByteArray data = file.read(blockSize);
            const int dataSize = data.size();
            if (dataSize != blockSize)
                data += QByteArray(blockSize - dataSize, 0);
            m_editor->addData(block, data);
            file.close();
        }
    }

    void provideNewRange(Core::IEditor *, quint64 offset) {
        open(m_fileName, offset);
    }

    void handleStartOfFileRequested(Core::IEditor *) {
        open(m_fileName, 0);
    }

    void handleEndOfFileRequested(Core::IEditor *) {
        open(m_fileName, QFileInfo(m_fileName).size() - 1);
    }

public:

    void setFilename(const QString &filename) {
        m_fileName = filename;
    }

    QString fileName() const { return m_fileName; }

    QString defaultPath() const { return QString(); }

    QString suggestedFileName() const { return QString(); }

    bool isModified() const { return m_editor->isMemoryView() ? false : m_editor->isModified(); }

    bool isReadOnly() const {
        if (m_editor->isMemoryView())
            return false;
        const QFileInfo fi(m_fileName);
        return !fi.isWritable();
    }

    bool isSaveAsAllowed() const { return true; }

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const {
        if (type == TypePermissions)
            return BehaviorSilent;
        if (type == TypeContents) {
            if (state == TriggerInternal && !isModified())
                return BehaviorSilent;
            return BehaviorAsk;
        }
        return BehaviorAsk;
    }

    void reload(ReloadFlag flag, ChangeType type) {
        if (flag == FlagIgnore)
            return;
        if (type == TypePermissions) {
            emit changed();
        } else {
            emit aboutToReload();
            if (open(m_fileName))
                emit reloaded();
        }
    }

private:
    BinEditor *m_editor;
    QString m_fileName;
};

class BinEditorInterface : public Core::IEditor
{
    Q_OBJECT
public:
    BinEditorInterface(BinEditor *editor)
    {
        m_editor = editor;
        m_file = new BinEditorFile(m_editor);
        m_context.add(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
        m_context.add(Constants::C_BINEDITOR);
        m_addressEdit = new QLineEdit;
        QRegExpValidator * const addressValidator
            = new QRegExpValidator(QRegExp(QLatin1String("[0-9a-fA-F]{1,16}")),
                m_addressEdit);
        m_addressEdit->setValidator(addressValidator);

        QHBoxLayout *l = new QHBoxLayout;
        QWidget *w = new QWidget;
        l->setMargin(0);
        l->setContentsMargins(0, 0, 5, 0);
        l->addStretch(1);
        l->addWidget(m_addressEdit);
        w->setLayout(l);

        m_toolBar = new QToolBar;
        m_toolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        m_toolBar->addWidget(w);

        connect(m_editor, SIGNAL(cursorPositionChanged(int)), this,
            SLOT(updateCursorPosition(int)));
        connect(m_file, SIGNAL(changed()), this, SIGNAL(changed()));
        connect(m_addressEdit, SIGNAL(editingFinished()), this,
            SLOT(jumpToAddress()));
        updateCursorPosition(m_editor->cursorPosition());
    }
    ~BinEditorInterface() {
        delete m_editor;
    }

    QWidget *widget() { return m_editor; }

    Core::Context context() const { return m_context; }

    bool createNew(const QString & /* contents */ = QString()) {
        m_editor->clear();
        m_file->setFilename(QString());
        return true;
    }
    bool open(const QString &fileName = QString()) {
        return m_file->open(fileName);
    }
    Core::IFile *file() { return m_file; }
    QString id() const { return QLatin1String(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID); }
    QString displayName() const { return m_displayName; }
    void setDisplayName(const QString &title) { m_displayName = title; emit changed(); }

    bool duplicateSupported() const { return false; }
    IEditor *duplicate(QWidget * /* parent */) { return 0; }

    QByteArray saveState() const { return QByteArray(); } // TODO
    bool restoreState(const QByteArray & /* state */) { return false; } // TODO

    QWidget *toolBar() { return m_toolBar; }

    bool isTemporary() const { return m_editor->isMemoryView(); }

private slots:
    void updateCursorPosition(int position) {
        m_addressEdit->setText(QString::number(m_editor->baseAddress() + position, 16));
    }

    void jumpToAddress() {
        m_editor->jumpToAddress(m_addressEdit->text().toULongLong(0, 16));
        updateCursorPosition(m_editor->cursorPosition());
    }

private:
    BinEditor *m_editor;
    QString m_displayName;
    BinEditorFile *m_file;
    Core::Context m_context;
    QToolBar *m_toolBar;
    QLineEdit *m_addressEdit;
};



///////////////////////////////// BinEditorFactory //////////////////////////////////

BinEditorFactory::BinEditorFactory(BinEditorPlugin *owner) :
    m_mimeTypes(QLatin1String(Constants::C_BINEDITOR_MIMETYPE)),
    m_owner(owner)
{
}

QString BinEditorFactory::id() const
{
    return QLatin1String(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
}

QString BinEditorFactory::displayName() const
{
    return tr(Constants::C_BINEDITOR_DISPLAY_NAME);
}

Core::IFile *BinEditorFactory::open(const QString &fileName)
{
    Core::EditorManager *em = Core::EditorManager::instance();
    Core::IEditor *iface = em->openEditor(fileName, id());
    return iface ? iface->file() : 0;
}

Core::IEditor *BinEditorFactory::createEditor(QWidget *parent)
{
    BinEditor *editor = new BinEditor(parent);
    m_owner->initializeEditor(editor);
    return editor->editor();
}

QStringList BinEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

///////////////////////////////// BinEditorPlugin //////////////////////////////////

BinEditorPlugin::BinEditorPlugin()
{
    m_undoAction = m_redoAction = m_copyAction = m_selectAllAction = 0;
}

BinEditorPlugin::~BinEditorPlugin()
{
}

QAction *BinEditorPlugin::registerNewAction(const QString &id, const QString &title)
{
    QAction *result = new QAction(title, this);
    Core::ICore::instance()->actionManager()->registerAction(result, id, m_context);
    return result;
}

QAction *BinEditorPlugin::registerNewAction(const QString &id,
                                            QObject *receiver,
                                            const char *slot,
                                            const QString &title)
{
    QAction *rc = registerNewAction(id, title);
    if (!rc)
        return 0;

    connect(rc, SIGNAL(triggered()), receiver, slot);
    return rc;
}

void BinEditorPlugin::initializeEditor(BinEditor *editor)
{
    BinEditorInterface *editorInterface = new BinEditorInterface(editor);
    QObject::connect(editor, SIGNAL(modificationChanged(bool)), editorInterface, SIGNAL(changed()));
    editor->setEditor(editorInterface);

    m_context.add(Constants::C_BINEDITOR);
    if (!m_undoAction) {
        m_undoAction      = registerNewAction(QLatin1String(Core::Constants::UNDO),
                                              this, SLOT(undoAction()),
                                              tr("&Undo"));
        m_redoAction      = registerNewAction(QLatin1String(Core::Constants::REDO),
                                              this, SLOT(redoAction()),
                                              tr("&Redo"));
        m_copyAction      = registerNewAction(QLatin1String(Core::Constants::COPY),
                                              this, SLOT(copyAction()));
        m_selectAllAction = registerNewAction(QLatin1String(Core::Constants::SELECTALL),
                                              this, SLOT(selectAllAction()));
    }

    // Font settings
    TextEditor::TextEditorSettings *settings = TextEditor::TextEditorSettings::instance();
    editor->setFontSettings(settings->fontSettings());
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            editor, SLOT(setFontSettings(TextEditor::FontSettings)));

    QObject::connect(editor, SIGNAL(undoAvailable(bool)), this, SLOT(updateActions()));
    QObject::connect(editor, SIGNAL(redoAvailable(bool)), this, SLOT(updateActions()));
    QObject::connect(editor, SIGNAL(copyAvailable(bool)), this, SLOT(updateActions()));

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    BinEditorFind *binEditorFind = new BinEditorFind(editor);
    aggregate->add(binEditorFind);
    aggregate->add(editor);
}

bool BinEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    Core::ICore *core = Core::ICore::instance();
    connect(core, SIGNAL(contextAboutToChange(Core::IContext *)),
        this, SLOT(updateCurrentEditor(Core::IContext *)));

    addAutoReleasedObject(new BinEditorFactory(this));
    return true;
}

void BinEditorPlugin::extensionsInitialized()
{
}

void BinEditorPlugin::updateCurrentEditor(Core::IContext *object)
{
    do {
        if (!object) {
            if (!m_currentEditor)
                return;

            m_currentEditor = 0;
            break;
        }
        BinEditor *editor = qobject_cast<BinEditor *>(object->widget());
        if (!editor) {
            if (!m_currentEditor)
                return;

            m_currentEditor = 0;
            break;
        }

        if (editor == m_currentEditor)
            return;

        m_currentEditor = editor;

    } while (false);
    updateActions();
}

void BinEditorPlugin::updateActions()
{
    bool hasEditor = (m_currentEditor != 0);
    if (m_selectAllAction)
        m_selectAllAction->setEnabled(hasEditor);
    if (m_undoAction)
        m_undoAction->setEnabled(m_currentEditor && m_currentEditor->isUndoAvailable());
    if (m_redoAction)
        m_redoAction->setEnabled(m_currentEditor && m_currentEditor->isRedoAvailable());
    if (m_copyAction)
        m_copyAction->setEnabled(m_currentEditor && m_currentEditor->hasSelection());
}

void BinEditorPlugin::undoAction()
{
    if (m_currentEditor)
        m_currentEditor->undo();
}

void BinEditorPlugin::redoAction()
{
    if (m_currentEditor)
        m_currentEditor->redo();
}

void BinEditorPlugin::copyAction()
{
    if (m_currentEditor)
        m_currentEditor->copy();
}

void BinEditorPlugin::selectAllAction()
{
    if (m_currentEditor)
        m_currentEditor->selectAll();
}


Q_EXPORT_PLUGIN(BinEditorPlugin)

#include "bineditorplugin.moc"
