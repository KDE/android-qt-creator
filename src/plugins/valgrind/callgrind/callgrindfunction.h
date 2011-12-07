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

#ifndef LIBVALGRIND_CALLGRIND_FUNCTION_H
#define LIBVALGRIND_CALLGRIND_FUNCTION_H

#include <QtCore/QMetaType>

QT_BEGIN_NAMESPACE
class QString;
template <typename T> class QVector;
template <typename T> class QSet;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {

class FunctionCall;
class CostItem;
class ParseData;

class Function
{
public:
    /// @p data the ParseData for the file this function was part of
    ///         required for the decompression of string data like function name etc.
    explicit Function(const ParseData *data);
    virtual ~Function();

    /// @return the compressed function name id
    qint64 nameId() const;
    /// @return the function name.
    QString name() const;
    /**
     * Set function name to internal string id @p id.
     * @see ParseData::stringForFunction()
     */
    void setName(qint64 id);

    /// @return the compressed file id
    qint64 fileId() const;
    /// @return the file path where this function was defined
    QString file() const;
    /**
     * Set function name to internal string id @p id.
     * @see ParseData::stringForFunction()
     */
    void setFile(qint64 id);

    /// @return the compressed object id
    qint64 objectId() const;
    /// @return the object where this function was defined
    QString object() const;
    /**
     * Set function name to internal string id @p id.
     * @see ParseData::stringForFunction()
     */
    void setObject(qint64 id);

    /**
     * @return a string representing the location of this function
     * It is a combination of file, object and line of the first CostItem.
     */
    QString location() const;

    /**
     * @return the line number of the function or -1 if not known
     */
    int lineNumber() const;

    /**
     * total accumulated self cost of @p event
     * @see ParseData::events()
     */
    quint64 selfCost(int event) const;
    QVector<quint64> selfCosts() const;

    /**
     * total accumulated inclusive cost of @p event
     * @see ParseData::events()
     */
    quint64 inclusiveCost(int event) const;

    /// calls from other functions to this function
    QVector<const FunctionCall *> incomingCalls() const;
    void addIncomingCall(const FunctionCall *call);
    /// @return how often this function was called in total
    quint64 called() const;

    /**
     * The detailed list of cost items, which could e.g. be used for
     * a detailed view of the function's source code annotated with
     * cost per line.
     */
    QVector<const CostItem *> costItems() const;

    /**
     * Add parsed @c CostItem @p item to this function.
     *
     * NOTE: The @c Function will take ownership.
     */
    void addCostItem(const CostItem *item);

    /**
     * Function calls from this function to others.
     */
    QVector<const FunctionCall *> outgoingCalls() const;
    void addOutgoingCall(const FunctionCall *call);

    /**
     * Gets called after all functions where looked up, required
     * to properly calculate inclusive cost of recursive functions
     * for example
     */
    void finalize();

protected:
    class Private;
    Private *d;

    explicit Function(Private *d);

private:
    Q_DISABLE_COPY(Function)
};

} // namespace Callgrind
} // namespace Valgrind

Q_DECLARE_METATYPE(const Valgrind::Callgrind::Function *);

#endif // LIBVALGRIND_CALLGRIND_FUNCTION_H
