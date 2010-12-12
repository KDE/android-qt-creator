/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
