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

#ifndef CHANGESET_H
#define CHANGESET_H

#include "utils_global.h"

#include <QtCore/QString>
#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QTextCursor)

namespace Utils {

class QTCREATOR_UTILS_EXPORT ChangeSet
{
public:
    struct EditOp {
        enum Type
        {
            Unset,
            Replace,
            Move,
            Insert,
            Remove,
            Flip,
            Copy
        };

        EditOp(): type(Unset), pos1(0), pos2(0), length1(0), length2(0) {}
        EditOp(Type t): type(t), pos1(0), pos2(0), length1(0), length2(0) {}

        Type type;
        int pos1;
        int pos2;
        int length1;
        int length2;
        QString text;
    };

    struct Range {
        Range()
            : start(0), end(0) {}

        Range(int start, int end)
            : start(start), end(end) {}

        int start;
        int end;
    };

public:
    ChangeSet();

    bool isEmpty() const;

    QList<EditOp> operationList() const;

    void clear();

    bool replace(const Range &range, const QString &replacement);
    bool remove(const Range &range);
    bool move(const Range &range, int to);
    bool flip(const Range &range1, const Range &range2);
    bool copy(const Range &range, int to);
    bool replace(int start, int end, const QString &replacement);
    bool remove(int start, int end);
    bool move(int start, int end, int to);
    bool flip(int start1, int end1, int start2, int end2);
    bool copy(int start, int end, int to);
    bool insert(int pos, const QString &text);

    bool hadErrors();

    void apply(QString *s);
    void apply(QTextCursor *textCursor);

private:
    // length-based API.
    bool replace_helper(int pos, int length, const QString &replacement);
    bool move_helper(int pos, int length, int to);
    bool remove_helper(int pos, int length);
    bool flip_helper(int pos1, int length1, int pos2, int length2);
    bool copy_helper(int pos, int length, int to);

    bool hasOverlap(int pos, int length);
    QString textAt(int pos, int length);

    void doReplace(const EditOp &replace, QList<EditOp> *replaceList);
    void convertToReplace(const EditOp &op, QList<EditOp> *replaceList);

    void apply_helper();

private:
    QString *m_string;
    QTextCursor *m_cursor;

    QList<EditOp> m_operationList;
    bool m_error;
};

} // namespace Utils

#endif // CHANGESET_H
