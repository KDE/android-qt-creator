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

#include "iassistprocessor.h"

using namespace TextEditor;

/*!
    \class TextEditor::IAssistProcessor
    \brief The IAssistProcessor class acts as an interface that actually computes an assist
    proposal.
    \ingroup CodeAssist

    \sa IAssistProposal, IAssistProvider
*/

IAssistProcessor::IAssistProcessor()
{}

IAssistProcessor::~IAssistProcessor()
{}

/*!
    \fn IAssistProposal *TextEditor::IAssistProcessor::perform(const IAssistInterface *interface)

    Computes a proposal and returns it. Access to the document is made through the \a interface.
    If this is an asynchronous processor the \a interface will be detached.

    The processor takes ownership of the interface. Also, one should be careful in the case of
    sharing data across asynchronous processors since there might be more than one instance of
    them computing a proposal at a particular time.

    \sa IAssistInterface::detach()
*/
