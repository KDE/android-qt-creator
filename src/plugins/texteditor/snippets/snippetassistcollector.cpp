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

#include "snippetassistcollector.h"
#include "snippetscollection.h"

#include <texteditor/texteditorconstants.h>
#include <texteditor/codeassist/basicproposalitem.h>

using namespace TextEditor;
using namespace Internal;

namespace {

void appendSnippets(QList<BasicProposalItem *> *items,
                    const QString &groupId,
                    const QIcon &icon,
                    int order)
{
    SnippetsCollection *collection = SnippetsCollection::instance();
    const int size = collection->totalActiveSnippets(groupId);
    for (int i = 0; i < size; ++i) {
        const Snippet &snippet = collection->snippet(i, groupId);
        BasicProposalItem *item = new BasicProposalItem;
        item->setText(snippet.trigger() + QLatin1Char(' ') + snippet.complement());
        item->setData(snippet.content());
        item->setDetail(snippet.generateTip());
        item->setIcon(icon);
        item->setOrder(order);
        items->append(item);
    }
}

} // anonymous


SnippetAssistCollector::SnippetAssistCollector(const QString &groupId, const QIcon &icon, int order)
    : m_groupId(groupId)
    , m_icon(icon)
    , m_order(order)
{}

SnippetAssistCollector::~SnippetAssistCollector()
{}

QList<BasicProposalItem *> SnippetAssistCollector::collect() const
{
    QList<BasicProposalItem *> snippets;
    appendSnippets(&snippets, m_groupId, m_icon, m_order);
    appendSnippets(&snippets, QLatin1String(Constants::TEXT_SNIPPET_GROUP_ID), m_icon, m_order);
    return snippets;
}
