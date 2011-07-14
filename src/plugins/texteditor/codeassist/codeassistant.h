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

#ifndef CODEASSISTANT_H
#define CODEASSISTANT_H

#include "assistenums.h"

#include <texteditor/texteditor_global.h>

#include <QtCore/QScopedPointer>

namespace TextEditor {

class CodeAssistantPrivate;
class IAssistProvider;
class BaseTextEditor;

class CodeAssistant
{
public:
    CodeAssistant();
    ~CodeAssistant();

    void configure(BaseTextEditor *textEditor);

    void process();
    void notifyChange();
    bool hasContext() const;
    void destroyContext();

    void invoke(AssistKind assistKind, IAssistProvider *provider = 0);

private:
    QScopedPointer<CodeAssistantPrivate> m_d;
};

} //TextEditor

#endif // CODEASSISTANT_H
