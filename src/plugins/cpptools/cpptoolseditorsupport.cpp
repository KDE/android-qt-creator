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

#include "cpptoolseditorsupport.h"
#include "cppmodelmanager.h"

#include <coreplugin/ifile.h>

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <AST.h>
#include <ASTVisitor.h>
#include <TranslationUnit.h>

#include <QtCore/QTimer>

using namespace CppTools::Internal;
using namespace CPlusPlus;

CppEditorSupport::CppEditorSupport(CppModelManager *modelManager)
    : QObject(modelManager),
      _modelManager(modelManager),
      _updateDocumentInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL)
{
    _revision = 0;

    _updateDocumentTimer = new QTimer(this);
    _updateDocumentTimer->setSingleShot(true);
    _updateDocumentTimer->setInterval(_updateDocumentInterval);
    connect(_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));
}

CppEditorSupport::~CppEditorSupport()
{ }

TextEditor::ITextEditor *CppEditorSupport::textEditor() const
{ return _textEditor; }

void CppEditorSupport::setTextEditor(TextEditor::ITextEditor *textEditor)
{
    _textEditor = textEditor;

    if (_textEditor) {
        connect(_textEditor, SIGNAL(contentsChanged()), this, SIGNAL(contentsChanged()));
        connect(this, SIGNAL(contentsChanged()), this, SLOT(updateDocument()));

        updateDocument();
    }
}

QString CppEditorSupport::contents()
{
    if (! _textEditor)
        return QString();
    else if (! _cachedContents.isEmpty())
        _cachedContents = _textEditor->contents();

    return _cachedContents;
}

unsigned CppEditorSupport::editorRevision() const
{
    if (_textEditor) {
        if (TextEditor::BaseTextEditorWidget *ed = qobject_cast<TextEditor::BaseTextEditorWidget *>(_textEditor->widget()))
            return ed->document()->revision();
    }

    return 0;
}

int CppEditorSupport::updateDocumentInterval() const
{ return _updateDocumentInterval; }

void CppEditorSupport::setUpdateDocumentInterval(int updateDocumentInterval)
{ _updateDocumentInterval = updateDocumentInterval; }

void CppEditorSupport::updateDocument()
{
    _revision = editorRevision();

    if (qobject_cast<TextEditor::BaseTextEditorWidget*>(_textEditor->widget()) != 0) {
        _modelManager->stopEditorSelectionsUpdate();
    }

    _updateDocumentTimer->start(_updateDocumentInterval);
}

void CppEditorSupport::updateDocumentNow()
{
    if (_documentParser.isRunning() || _revision != editorRevision()) {
        _updateDocumentTimer->start(_updateDocumentInterval);
    } else {
        _updateDocumentTimer->stop();

        QStringList sourceFiles(_textEditor->file()->fileName());
        _cachedContents = _textEditor->contents().toUtf8();
        _documentParser = _modelManager->refreshSourceFiles(sourceFiles);
    }
}

