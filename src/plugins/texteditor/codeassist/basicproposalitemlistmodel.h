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

#ifndef BASICPROPOSALITEMLISTMODEL_H
#define BASICPROPOSALITEMLISTMODEL_H

#include "igenericproposalmodel.h"

#include <texteditor/texteditor_global.h>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QPair>

namespace TextEditor {

class BasicProposalItem;

class TEXTEDITOR_EXPORT BasicProposalItemListModel : public IGenericProposalModel
{
public:
    BasicProposalItemListModel();
    BasicProposalItemListModel(const QList<BasicProposalItem *> &items);
    virtual ~BasicProposalItemListModel();

    virtual void reset();
    virtual int size() const;
    virtual QString text(int index) const;
    virtual QIcon icon(int index) const;
    virtual QString detail(int index) const;
    virtual int persistentId(int index) const;
    virtual void removeDuplicates();
    virtual void filter(const QString &prefix);
    virtual bool isSortable() const;
    virtual void sort();
    virtual bool supportsPrefixExpansion() const;
    virtual QString proposalPrefix() const;
    virtual bool keepPerfectMatch(AssistReason reason) const;
    virtual IAssistProposalItem *proposalItem(int index) const;

    void loadContent(const QList<BasicProposalItem *> &items);

protected:
    typedef QList<BasicProposalItem *>::iterator ItemIterator;
    QPair<ItemIterator, ItemIterator> currentItems();

private:
    void mapPersistentIds();

    QHash<QString, int> m_idByText;
    QList<BasicProposalItem *> m_originalItems;
    QList<BasicProposalItem *> m_currentItems;
};

} // TextEditor

#endif // BASICPROPOSALITEMLISTMODEL_H
