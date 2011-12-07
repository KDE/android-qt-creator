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

#include "iassistinterface.h"

using namespace TextEditor;

/*!
    \class TextEditor::IAssistInterface
    \brief The IAssistInterface class acts as an interface for providing access to the document
    from which a proposal is computed.
    \ingroup CodeAssist

    This interface existis in order to avoid a direct dependency on the text editor. This is
    particularly important and safer for asynchronous providers, since in such cases computation
    of the proposal is not done in the GUI thread.

    In general this API tries to be as decoupled as possible from the base text editor.
    This is in order to make the design a bit more generic and allow code assist to be
    pluggable into different types of documents (there are still issues to be treated).

    This class is part of the CodeAssist API.

    \sa IAssistProposal, IAssistProvider, IAssistProcessor
*/

IAssistInterface::IAssistInterface()
{}

IAssistInterface::~IAssistInterface()
{}

/*!
    \fn int TextEditor::IAssistInterface::position() const

    Returns the cursor position.
*/

/*!
    \fn QChar TextEditor::IAssistInterface::characterAt(int position) const

    Returns the character at \a position.
*/

/*!
    \fn QString TextEditor::IAssistInterface::textAt(int position, int length) const

    Returns the text at \a position with the given \a length.
*/

/*!
    \fn const Core::IFile *TextEditor::IAssistInterface::file() const

    Returns the file associated.
*/

/*!
    \fn QTextDocument *TextEditor::IAssistInterface::document() const
    Returns the document.
*/

/*!
    \fn void TextEditor::IAssistInterface::detach(QThread *destination)

    Detaches the interface. If it is necessary to take any special care in order to allow
    this interface to be run in a separate thread \a destination this needs to be done
    in this method.
*/

/*!
    \fn AssistReason TextEditor::IAssistInterface::reason() const

    The reason which triggered the assist.
*/
