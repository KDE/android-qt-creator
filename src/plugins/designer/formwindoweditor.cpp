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

#include "formwindoweditor.h"
#include "formwindowfile.h"
#include "designerconstants.h"
#include "resourcehandler.h"
#include "designerxmleditor.h"
#include <widgethost.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/plaintexteditor.h>

#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#if QT_VERSION >= 0x050000
#    include <QtDesigner/QDesignerFormWindowInterface>
#    include <QtCore/QBuffer>
#else
#    include "qt_private/formwindowbase_p.h"
#endif

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtGui/QApplication>

namespace Designer {

struct FormWindowEditorPrivate
{
    FormWindowEditorPrivate(Internal::DesignerXmlEditor *editor,
                            QDesignerFormWindowInterface *form)
        : m_textEditor(editor), m_file(form)
    {}

    TextEditor::PlainTextEditor m_textEditor;
    Internal::FormWindowFile m_file;
};

FormWindowEditor::FormWindowEditor(Internal::DesignerXmlEditor *editor,
                                   QDesignerFormWindowInterface *form,
                                   QObject *parent) :
    Core::IEditor(parent),
    d(new FormWindowEditorPrivate(editor, form))
{
    setContext(Core::Context(Designer::Constants::K_DESIGNER_XML_EDITOR_ID,
                             Designer::Constants::C_DESIGNER_XML_EDITOR));
    setWidget(d->m_textEditor.widget());

    connect(form, SIGNAL(changed()), this, SIGNAL(changed()));
    // Revert to saved/load externally modified files.
    connect(&d->m_file, SIGNAL(reload(QString*,QString)), this, SLOT(slotOpen(QString*,QString)));
    // Force update of open editors model.
    connect(&d->m_file, SIGNAL(saved()), this, SIGNAL(changed()));
    connect(&d->m_file, SIGNAL(changed()), this, SIGNAL(changed()));
}

FormWindowEditor::~FormWindowEditor()
{
    delete d;
}

bool FormWindowEditor::createNew(const QString &contents)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormWindowEditor::createNew" << contents.size();

    syncXmlEditor(QString());

    QDesignerFormWindowInterface *form = d->m_file.formWindow();
    QTC_ASSERT(form, return false);

    if (contents.isEmpty())
        return false;

    // If we have an override cursor, reset it over Designer loading,
    // should it pop up messages about missing resources or such.
    const bool hasOverrideCursor = QApplication::overrideCursor();
    QCursor overrideCursor;
    if (hasOverrideCursor) {
        overrideCursor = QCursor(*QApplication::overrideCursor());
        QApplication::restoreOverrideCursor();
    }

#if QT_VERSION >= 0x050000
    const bool success = form->setContents(contents);
#else
    form->setContents(contents);
    const bool success = form->mainContainer() != 0;
#endif

    if (hasOverrideCursor)
        QApplication::setOverrideCursor(overrideCursor);

    if (!success)
        return false;

    syncXmlEditor(contents);
    d->m_file.setFileName(QString());
    d->m_file.setShouldAutoSave(false);
    return true;
}

void FormWindowEditor::slotOpen(QString *errorString, const QString &fileName)
{
    open(errorString, fileName, fileName);
}

bool FormWindowEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormWindowEditor::open" << fileName;

    QDesignerFormWindowInterface *form = d->m_file.formWindow();
    QTC_ASSERT(form, return false);

    if (fileName.isEmpty()) {
        setDisplayName(tr("untitled"));
        return true;
    }

    const QFileInfo fi(fileName);
    const QString absfileName = fi.absoluteFilePath();

    QString contents;
    if (d->m_file.read(absfileName, &contents, errorString) != Utils::TextFileFormat::ReadSuccess)
        return false;

    form->setFileName(absfileName);
#if QT_VERSION >= 0x050000
    const QByteArray contentsBA = contents.toUtf8();
    QBuffer str(&contentsBA);
    str.open(QIODevice::ReadOnly);
    if (!form->setContents(&str, errorString))
        return false;
#else
    form->setContents(contents);
    if (!form->mainContainer())
        return false;
#endif
    form->setDirty(fileName != realFileName);
    syncXmlEditor(contents);

    setDisplayName(fi.fileName());
    d->m_file.setFileName(absfileName);
    d->m_file.setShouldAutoSave(false);

    if (Internal::ResourceHandler *rh = qFindChild<Designer::Internal::ResourceHandler*>(form))
        rh->updateResources();

    emit changed();

    return true;
}

void FormWindowEditor::syncXmlEditor()
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormWindowEditor::syncXmlEditor" << d->m_file.fileName();
    syncXmlEditor(contents());
}

void FormWindowEditor::syncXmlEditor(const QString &contents)
{
    d->m_textEditor.editorWidget()->setPlainText(contents);
    d->m_textEditor.editorWidget()->setReadOnly(true);
    static_cast<TextEditor::PlainTextEditorWidget *>
            (d->m_textEditor.editorWidget())->configure(file()->mimeType());
}

Core::IFile *FormWindowEditor::file()
{
    return &d->m_file;
}

QString FormWindowEditor::id() const
{
    return QLatin1String(Designer::Constants::K_DESIGNER_XML_EDITOR_ID);
}

QString FormWindowEditor::displayName() const
{
    return d->m_textEditor.displayName();
}

void FormWindowEditor::setDisplayName(const QString &title)
{
    d->m_textEditor.setDisplayName(title);
    emit changed();
}

bool FormWindowEditor::duplicateSupported() const
{
    return false;
}

Core::IEditor *FormWindowEditor::duplicate(QWidget *)
{
    return 0;
}

QByteArray FormWindowEditor::saveState() const
{
    return d->m_textEditor.saveState();
}

bool FormWindowEditor::restoreState(const QByteArray &state)
{
    return d->m_textEditor.restoreState(state);
}

bool FormWindowEditor::isTemporary() const
{
    return false;
}

QWidget *FormWindowEditor::toolBar()
{
    return 0;
}

QString FormWindowEditor::contents() const
{
#if QT_VERSION >= 0x050000    // TODO: No warnings about spacers here
    const QDesignerFormWindowInterface *fw = d->m_file.formWindow();
    QTC_ASSERT(fw, return QString());
    return fw->contents();
#else
    // No warnings about spacers here
    const qdesigner_internal::FormWindowBase *fw = qobject_cast<const qdesigner_internal::FormWindowBase *>(d->m_file.formWindow());
    QTC_ASSERT(fw, return QString());
    return fw->fileContents();
#endif
}

TextEditor::BaseTextDocument *FormWindowEditor::textDocument()
{
    return qobject_cast<TextEditor::BaseTextDocument*>(d->m_textEditor.file());
}

TextEditor::PlainTextEditor *FormWindowEditor::textEditor()
{
    return &d->m_textEditor;
}

QString FormWindowEditor::preferredModeType() const
{
    return QLatin1String(Core::Constants::MODE_DESIGN_TYPE);
}

} // namespace Designer

