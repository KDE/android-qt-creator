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

#include "iassistproposalwidget.h"

using namespace TextEditor;

/*!
    \class TextEditor::IAssistProposalWidget
    \brief The IAssistProposalWidget class acts as an interface for widgets that display
    assist proposals.
    \ingroup CodeAssist

    Known implenters of this interface are FunctionHintProposalWidget and GenericProposalWidget.
    The former is recommeded to be used when assisting function calls constructs (overloads
    and parameters) while the latter is quite generic so that it could be used to propose
    snippets, refactoring operations (quickfixes), and contextual content (the member of class
    or a string existent in the document, for example).

    In general this API tries to be as decoupled as possible from the base text editor.
    This is in order to make the design a bit more generic and allow code assist to be
    pluggable into different types of documents (there are still issues to be treated).

    This is class is part of the CodeAssist API.

    \sa IAssistProposal
*/

IAssistProposalWidget::IAssistProposalWidget()
    : QFrame(0, Qt::Popup)
{}

IAssistProposalWidget::~IAssistProposalWidget()
{}

/*!
    \fn void TextEditor::IAssistProposalWidget::setAssistant(CodeAssistant *assistant)

    Sets the code assistant which is the owner of this widget. This is used so that the code
    assistant can be notified when changes on the underlying widget happen.
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::setReason(AssistReason reason)

    Sets the reason which triggered the assist.
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::setUnderlyingWidget(const QWidget *underlyingWidget)

    Sets the underlying widget upon which this proposal operates.
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::setModel(IAssistModel *model)

    Sets the model.
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::setDisplayRect(const QRect &rect)

    Sets the \a rect on which this widget should be displayed.
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::showProposal(const QString &prefix)

    Shows the proposal. The \a prefix is the string comprised from the character at the base
    position of the proposal until the character immediately after the cursor at the moment
    the proposal is displayed.

    \sa IAssistProposal::basePosition()
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::updateProposal(const QString &prefix)

    Updates the proposal base on the give \a prefix.

    \sa showProposal()
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::closeProposal()

    Closes the proposal.
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::setIsSynchronized(bool isSync)

    Sets whether this widget is synchronized. If a widget is synchronized it means that from
    the moment a proposal started being computed until the moment it is actually displayed,
    there was no content input into the underlying widget.

    A widget is not synchronized in the case a proposal is computed in a separate thread and
    in the meanwhile (while it is still being processed) content is input into the underlying
    widget.
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::prefixExpanded(const QString &newPrefix)

    The signal is emitted whenever this widget automatically expands the prefix of the proposal.
    This can happen if all available proposal items share the same prefix and if the proposal's
    model supports prefix expansion.

    \sa IGenericProposalModel::supportsPrefixExpansion()
*/

/*!
    \fn void TextEditor::IAssistProposalWidget::proposalItemActivated(IAssistProposalItem *proposalItem)

    This signal is emitted whenever \a proposalItem is chosen to be applied.
*/
