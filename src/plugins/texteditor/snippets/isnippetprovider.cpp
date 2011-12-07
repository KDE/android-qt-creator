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

#include "isnippetprovider.h"

using namespace TextEditor;

/*!
    \group Snippets
    \title Snippets for Editors

    Snippets typically consist of chunks of code in a programming language (although they
    can also be plain text) which you would like to avoid re-typing every time. They
    are triggered in the same way as the completions (see \l{CodeAssist}{Providing
    Code Assist}).

    In order to create a new group of snippets two steps are necessary:
    \list
        \o Implement the TextEditor::ISnippetProvider interface and register it in
        the extension system.
        \o Create an XML configuration file and place it in the
        /share/qtcreator/snippets directory. As an example of the file format
        please take a look at the already available ones. The meaning and consistency rules
        of the fields are described below:
        \list
            \o group - This is the group in which the snippet belongs in the user interface.
            It must match TextEditor::ISnippetProvider::groupId().
            \o id - A unique string that identifies this snippet among all others available.
            The recommended practice is to prefix it with the group so it is easier to have
            such control on a file level.
            \o trigger - The sequence of characters to be compared by the completion engine
            in order to display this snippet as a code assist proposal.
            \o complement - Additional information that is displayed in the code assist
            proposal so it is possible to disambiguate similar snippets that have the
            same trigger.
        \endlist
    \endlist

    All XML configuration files found in the directory mentioned above are parsed by
    Qt Creator. However, only the ones which are associated with known groups (specified
    by a provider) are taken into consideration.
*/

/*!
    \class TextEditor::ISnippetProvider
    \brief The ISnippetProvider class acts as an interface for providing groups of snippets.
    \ingroup Snippets

    Known implementors of this interface are the CppSnippetProvider, the QmlJSSnippetProvider,
    and the PlainTextSnippetProvider.
*/

ISnippetProvider::ISnippetProvider() : QObject()
{}

ISnippetProvider::~ISnippetProvider()
{}

/*!
    \fn QString TextEditor::ISnippetProvider::groupId() const

    Returns the unique group id to which this provider is associated.
*/

/*!
    \fn QString TextEditor::ISnippetProvider::displayName() const

    Returns the name to be displayed in the user interface for snippets that belong to the group
    associated with this provider.
*/

/*!
    \fn void TextEditor::ISnippetProvider::decorateEditor(SnippetEditorWidget *editor) const

    This is a hook which allows you to apply customizations such as highlighting or indentation
    to the snippet editor.
*/
