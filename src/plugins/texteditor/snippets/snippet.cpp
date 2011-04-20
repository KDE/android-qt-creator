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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "snippet.h"

#include <QtCore/QLatin1Char>
#include <QtCore/QLatin1String>
#include <QtGui/QTextDocument>

using namespace TextEditor;

const QChar Snippet::kVariableDelimiter(QLatin1Char('$'));

Snippet::Snippet(const QString &groupId, const QString &id) :
    m_isRemoved(false), m_isModified(false), m_groupId(groupId), m_id(id)
{}

Snippet::~Snippet()
{}

const QString &Snippet::id() const
{
    return m_id;
}

const QString &Snippet::groupId() const
{
    return m_groupId;
}

bool Snippet::isBuiltIn() const
{
    return !m_id.isEmpty();
}

void Snippet::setTrigger(const QString &trigger)
{
    m_trigger = trigger;
}

const QString &Snippet::trigger() const
{
    return m_trigger;
}

void Snippet::setContent(const QString &content)
{
    m_content = content;
}

const QString &Snippet::content() const
{
    return m_content;
}

void Snippet::setComplement(const QString &complement)
{
    m_complement = complement;
}

const QString &Snippet::complement() const
{
    return m_complement;
}

void Snippet::setIsRemoved(bool removed)
{
    m_isRemoved = removed;
}

bool Snippet::isRemoved() const
{
    return m_isRemoved;
}

void Snippet::setIsModified(bool modified)
{
    m_isModified = modified;
}

bool Snippet::isModified() const
{
    return m_isModified;
}

QString Snippet::generateTip() const
{
    static const QLatin1Char kNewLine('\n');
    static const QLatin1Char kSpace(' ');
    static const QLatin1String kBr("<br>");
    static const QLatin1String kNbsp("&nbsp;");
    static const QLatin1String kNoBr("<nobr>");
    static const QLatin1String kOpenBold("<b>");
    static const QLatin1String kCloseBold("</b>");
    static const QLatin1String kEllipsis("...");

    QString escapedContent(Qt::escape(m_content));
    escapedContent.replace(kNewLine, kBr);
    escapedContent.replace(kSpace, kNbsp);

    QString tip(kNoBr);
    int count = 0;
    for (int i = 0; i < escapedContent.count(); ++i) {
        if (escapedContent.at(i) != kVariableDelimiter) {
            tip += escapedContent.at(i);
            continue;
        }
        if (++count % 2) {
            tip += kOpenBold;
        } else {
            if (escapedContent.at(i-1) == kVariableDelimiter)
                tip += kEllipsis;
            tip += kCloseBold;
        }
    }

    return tip;
}
