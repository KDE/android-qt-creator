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

#ifndef QMLJSCOMPLETIONASSIST_H
#define QMLJSCOMPLETIONASSIST_H

#include "qmljseditor.h"

#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/snippets/snippetassistcollector.h>
#include <texteditor/codeassist/defaultassistinterface.h>

#include <QtCore/QStringList>
#include <QtCore/QScopedPointer>
#include <QtCore/QVariant>
#include <QtGui/QIcon>

namespace QmlJS {
class Value;
}

namespace QmlJSEditor {
namespace Internal {

class QmlJSCompletionAssistInterface;

class QmlJSAssistProposalItem : public TextEditor::BasicProposalItem
{
public:
    virtual bool prematurelyApplies(const QChar &c) const;
    virtual void applyContextualContent(TextEditor::BaseTextEditor *editor,
                                        int basePosition) const;
};


class QmlJSAssistProposalModel : public TextEditor::BasicProposalItemListModel
{
public:
    QmlJSAssistProposalModel(const QList<TextEditor::BasicProposalItem *> &items)
        : TextEditor::BasicProposalItemListModel(items)
    {}

    virtual void sort();
    virtual bool keepPerfectMatch(TextEditor::AssistReason reason) const;
};


class QmlJSCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
public:
    virtual bool supportsEditor(const Core::Id &editorId) const;
    virtual TextEditor::IAssistProcessor *createProcessor() const;

    virtual int activationCharSequenceLength() const;
    virtual bool isActivationCharSequence(const QString &sequence) const;
    virtual bool isContinuationChar(const QChar &c) const;
};


class QmlJSCompletionAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    QmlJSCompletionAssistProcessor();
    virtual ~QmlJSCompletionAssistProcessor();

    virtual TextEditor::IAssistProposal *perform(const TextEditor::IAssistInterface *interface);

private:
    TextEditor::IAssistProposal *createContentProposal() const;
    TextEditor::IAssistProposal *createHintProposal(
            const QString &functionName, const QStringList &namedArguments,
            int optionalNamedArguments, bool isVariadic) const;

    bool acceptsIdleEditor() const;

    bool completeUrl(const QString &relativeBasePath, const QString &urlString);
    bool completeFileName(const QString &relativeBasePath,
                          const QString &fileName,
                          const QStringList &patterns = QStringList());

    int m_startPosition;
    QScopedPointer<const QmlJSCompletionAssistInterface> m_interface;
    QList<TextEditor::BasicProposalItem *> m_completions;
    TextEditor::SnippetAssistCollector m_snippetCollector;
    const TextEditor::IAssistProvider *m_provider;
};


class QmlJSCompletionAssistInterface : public TextEditor::DefaultAssistInterface
{
public:
    QmlJSCompletionAssistInterface(QTextDocument *document,
                                   int position,
                                   Core::IFile *file,
                                   TextEditor::AssistReason reason,
                                   const SemanticInfo &info);
    const SemanticInfo &semanticInfo() const;
    const QIcon &fileNameIcon() const { return m_darkBlueIcon; }
    const QIcon &keywordIcon() const { return m_darkYellowIcon; }
    const QIcon &symbolIcon() const { return m_darkCyanIcon; }

private:
    SemanticInfo m_semanticInfo;
    QIcon m_darkBlueIcon;
    QIcon m_darkYellowIcon;
    QIcon m_darkCyanIcon;
};

} // Internal
} // QmlJSEditor

#endif // QMLJSCOMPLETIONASSIST_H
