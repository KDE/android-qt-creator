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

#ifndef QMLJSQUICKFIXASSIST_H
#define QMLJSQUICKFIXASSIST_H

#include "qmljseditor.h"

#include <qmljstools/qmljsrefactoringchanges.h>

#include <texteditor/codeassist/defaultassistinterface.h>
#include <texteditor/codeassist/quickfixassistprovider.h>
#include <texteditor/codeassist/quickfixassistprocessor.h>

namespace QmlJSEditor {
namespace Internal {

class QmlJSQuickFixAssistInterface : public TextEditor::DefaultAssistInterface
{
public:
    QmlJSQuickFixAssistInterface(QmlJSTextEditorWidget *editor, TextEditor::AssistReason reason);
    virtual ~QmlJSQuickFixAssistInterface();

    const SemanticInfo &semanticInfo() const;
    QmlJSTools::QmlJSRefactoringFilePtr currentFile() const;
    QmlJSTextEditorWidget *editor() const;

private:
    QmlJSTextEditorWidget *m_editor;
    SemanticInfo m_semanticInfo;
    QmlJSTools::QmlJSRefactoringFilePtr m_currentFile;
};


class QmlJSQuickFixProcessor : public TextEditor::QuickFixAssistProcessor
{
public:
    QmlJSQuickFixProcessor(const TextEditor::IAssistProvider *provider);
    virtual ~QmlJSQuickFixProcessor();

    virtual const TextEditor::IAssistProvider *provider() const;

private:
    const TextEditor::IAssistProvider *m_provider;
};


class QmlJSQuickFixAssistProvider : public TextEditor::QuickFixAssistProvider
{
public:
    QmlJSQuickFixAssistProvider();
    virtual ~QmlJSQuickFixAssistProvider();

    virtual bool supportsEditor(const Core::Id &editorId) const;
    virtual TextEditor::IAssistProcessor *createProcessor() const;

    virtual QList<TextEditor::QuickFixFactory *> quickFixFactories() const;
};

} // Internal
} // QmlJSEditor

#endif // QMLJSQUICKFIXASSIST_H
