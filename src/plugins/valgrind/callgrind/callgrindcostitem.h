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

#ifndef LIBVALGRIND_CALLGRIND_COSTITEM_H
#define LIBVALGRIND_CALLGRIND_COSTITEM_H

#include <QtCore/QtGlobal>

QT_BEGIN_NAMESPACE
class QString;
template<typename T> class QVector;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {

class FunctionCall;
class ParseData;

/**
 * This class represents the cost(s) at given position(s).
 */
class CostItem
{
public:
    /// @p data the file data this cost item was parsed in.
    ///         required for decompression of string data like differing source file information
    explicit CostItem(ParseData *data);
    ~CostItem();

    /**
     * Position data for the given position-index @p posIdx
     * @see ParseData::positions()
     */
    quint64 position(int posIdx) const;
    void setPosition(int posIdx, quint64 position);
    QVector<quint64> positions() const;

    /**
     * Cost data for the given event-index @p event
     * @see ParseData::events()
     */
    quint64 cost(int event) const;
    void setCost(int event, quint64 cost);
    QVector<quint64> costs() const;

    /**
     * If this cost item represents a function call, this will return the @c Callee.
     * Otherwise zero will be returned.
     */
    const FunctionCall *call() const;
    ///NOTE: @c CostItem will take ownership
    void setCall(const FunctionCall *call);

    /**
     * If this cost item represents a jump to a different file, this will
     * return the path to that file. The string will be empty otherwise.
     */
    QString differingFile() const;
    /// @return compressed file id or -1 if none is set
    qint64 differingFileId() const;
    void setDifferingFile(qint64 fileId);

private:
    Q_DISABLE_COPY(CostItem)

    class Private;
    Private *d;
};

} // namespace Callgrind
} // namespace Valgrind

#endif // LIBVALGRIND_CALLGRIND_COSTITEM_H
