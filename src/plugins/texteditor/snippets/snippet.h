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

#ifndef SNIPPET_H
#define SNIPPET_H

#include <texteditor/texteditor_global.h>

#include <QtCore/QChar>
#include <QtCore/QString>

namespace TextEditor {

class TEXTEDITOR_EXPORT Snippet
{
public:
    Snippet(const QString &groupId = QString(), const QString &id = QString());
    ~Snippet();

    const QString &id() const;
    const QString &groupId() const;

    bool isBuiltIn() const;

    void setTrigger(const QString &trigger);
    const QString &trigger() const;

    void setContent(const QString &content);
    const QString &content() const;

    void setComplement(const QString &complement);
    const QString &complement() const;

    void setIsRemoved(bool removed);
    bool isRemoved() const;

    void setIsModified(bool modified);
    bool isModified() const;

    QString generateTip() const;

    static const QChar kVariableDelimiter;

private:
    bool m_isRemoved;
    bool m_isModified;
    QString m_groupId;
    QString m_id; // Only built-in snippets have an id.
    QString m_trigger;
    QString m_content;
    QString m_complement;
};

} // TextEditor

#endif // SNIPPET_H
