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

#ifndef LIBVALGRIND_CALLGRINDCYCLEDETECTION_H
#define LIBVALGRIND_CALLGRINDCYCLEDETECTION_H

#include <QtCore/QHash>
#include <QtCore/QStack>

namespace Valgrind {
namespace Callgrind {

class Function;
class ParseData;

namespace Internal {

/**
 * Implementation of Tarjan's strongly connected components algorithm, to find function cycles,
 * as suggested by the GProf paper:
 *
 * ``gprof: A Call Graph Execution Profiler'', by S. Graham,  P.  Kessler,
 *      M.  McKusick; Proceedings of the SIGPLAN '82 Symposium on Compiler Construction,
 * SIGPLAN Notices, Vol. 17, No   6, pp. 120-126, June 1982.
 */
class CycleDetection
{
public:
    explicit CycleDetection(ParseData *data);
    QVector<const Function *> run(const QVector<const Function *> &input);

private:
    ParseData *m_data;

    struct Node {
        int dfs;
        int lowlink;
        const Function *function;
    };

    void tarjan(Node *node);
    void tarjanForChildNode(Node *node, Node *childNode);

    QHash<const Function *, Node *> m_nodes;
    QStack<Node *> m_stack;
    QVector<const Function *> m_ret;
    int m_depth;

    int m_cycle;
};

} // namespace Internal

} // namespace Callgrind
} // namespace Valgrind

#endif // LIBVALGRIND_CALLGRINDCYCLEDETECTION_H
