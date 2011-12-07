/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#ifndef LIBVALGRIND_PROTOCOL_ERROR_H
#define LIBVALGRIND_PROTOCOL_ERROR_H

#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QString;
template <typename T> class QVector;
QT_END_NAMESPACE

namespace Valgrind {
namespace XmlProtocol {

class Frame;
class Stack;
class Suppression;

/**
 * Error kinds, specific to memcheck
 */
enum MemcheckErrorKind
{
    InvalidFree,
    MismatchedFree,
    InvalidRead,
    InvalidWrite,
    InvalidJump,
    Overlap,
    InvalidMemPool,
    UninitCondition,
    UninitValue,
    SyscallParam,
    ClientCheck,
    Leak_DefinitelyLost,
    Leak_PossiblyLost,
    Leak_StillReachable,
    Leak_IndirectlyLost,
    MemcheckErrorKindCount
};

enum PtrcheckErrorKind
{
    SorG,
    Heap,
    Arith,
    SysParam
};

enum HelgrindErrorKind
{
    Race,
    UnlockUnlocked,
    UnlockForeign,
    UnlockBogus,
    PthAPIerror,
    LockOrder,
    Misc
};

class Error
{
public:
    Error();
    ~Error();

    Error(const Error &other);

    Error &operator=(const Error &other);
    void swap(Error &other);

    bool operator==(const Error &other) const;
    bool operator!=(const Error &other) const;

    qint64 unique() const;
    void setUnique(qint64 unique);

    qint64 tid() const;
    void setTid(qint64);

    QString what() const;
    void setWhat(const QString &what);

    int kind() const;
    void setKind(int kind);

    QVector<Stack> stacks() const;
    void setStacks(const QVector<Stack> &stacks);

    Suppression suppression() const;
    void setSuppression(const Suppression &suppression);

    //memcheck
    quint64 leakedBytes() const;
    void setLeakedBytes(quint64);

    qint64 leakedBlocks() const;
    void setLeakedBlocks(qint64 blocks);

    //helgrind
    qint64 helgrindThreadId() const;
    void setHelgrindThreadId( qint64 threadId );

    QString toXml() const;

private:
    class Private;
    QSharedDataPointer<Private> d;
};

} // namespace XmlProtocol
} // namespace Valgrind

Q_DECLARE_METATYPE(Valgrind::XmlProtocol::Error)

#endif // LIBVALGRIND_PROTOCOL_ERROR_H
