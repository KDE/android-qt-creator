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

#ifndef SNIPPETSCOLLECTION_H
#define SNIPPETSCOLLECTION_H

#include "snippet.h"

#include <QtCore/QVector>
#include <QtCore/QStringList>
#include <QtCore/QHash>
#include <QtCore/QXmlStreamWriter>

namespace TextEditor {
namespace Internal {

// Characteristics of this collection:
// - Store snippets by group.
// - Keep groups of snippets sorted.
// - Allow snippet insertion/replacement based on a hint.
// - Allow modification of snippet members that are not sorting keys.
// - Track removed/modified built-in snippets.
// - Provide fast index access.
// - Not thread-safe.

class SnippetsCollection : public QObject
{
    Q_OBJECT
public:
    virtual ~SnippetsCollection();

    static SnippetsCollection *instance();

    class Hint
    {
        friend class SnippetsCollection;
    public:
        int index() const;
    private:
        explicit Hint(int index);
        Hint(int index, QList<Snippet>::iterator it);
        int m_index;
        QList<Snippet>::iterator m_it;
    };

    void insertSnippet(const Snippet &snippet);
    void insertSnippet(const Snippet &snippet, const Hint &hint);
    Hint computeInsertionHint(const Snippet &snippet);

    // Replace snippets only within the same group.
    void replaceSnippet(int index, const Snippet &snippet);
    void replaceSnippet(int index, const Snippet &snippet, const Hint &hint);
    Hint computeReplacementHint(int index, const Snippet &snippet);

    void removeSnippet(int index, const QString &groupId);
    void restoreRemovedSnippets(const QString &groupId);

    void setSnippetContent(int index, const QString &groupId, const QString &content);

    const Snippet &snippet(int index, const QString &groupId) const;
    Snippet revertedSnippet(int index, const QString &groupId) const;

    void reset(const QString &groupId);

    int totalActiveSnippets(const QString &groupId) const;
    int totalSnippets(const QString &groupId) const;

    QList<QString> groupIds() const;

    void reload();
    void synchronize();

private slots:
    void identifyGroups();

private:
    SnippetsCollection();
    Q_DISABLE_COPY(SnippetsCollection)

    int groupIndex(const QString &groupId) const;
    bool isGroupKnown(const QString &groupId) const;

    void clearSnippets();
    void clearSnippets(int groupIndex);

    void updateActiveSnippetsEnd(int groupIndex);

    QList<Snippet> readXML(const QString &fileName, const QString &snippetId = QString()) const;
    void writeSnippetXML(const Snippet &snippet, QXmlStreamWriter *writer) const;

    QList<Snippet> allBuiltInSnippets() const;

    static const QLatin1String kSnippet;
    static const QLatin1String kSnippets;
    static const QLatin1String kTrigger;
    static const QLatin1String kId;
    static const QLatin1String kComplement;
    static const QLatin1String kGroup;
    static const QLatin1String kRemoved;
    static const QLatin1String kModified;

    // Built-in snippets are specified in XMLs distributed in a system's folder. Snippets
    // created or modified/removed (if they are built-ins) by the user are stored in user's
    // folder.
    QString m_userSnippetsPath;
    QString m_userSnippetsFile;
    QStringList m_builtInSnippetsFiles;

    // Snippets for each group are kept in a list. However, not all of them are necessarily
    // active. Specifically, removed built-in snippets are kept as the last ones (for each
    // group there is a iterator that marks the logical end).
    QVector<QList<Snippet> > m_snippets;
    QVector<QList<Snippet>::iterator> m_activeSnippetsEnd;

    QHash<QString, int> m_groupIndexById;
};

} // Internal
} // TextEditor

#endif // SNIPPETSCOLLECTION_H
