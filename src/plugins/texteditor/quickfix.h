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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef TEXTEDITORQUICKFIX_H
#define TEXTEDITORQUICKFIX_H

#include "texteditor_global.h"

#include <QtCore/QString>
#include <QtCore/QMetaType>
#include <QtCore/QSharedPointer>

namespace TextEditor {

class IAssistInterface;

/*!
    Class to perform a single quick-fix.

    Quick-fix operations cannot be copied, and must be passed around as explicitly
    shared pointers ( QuickFixOperation::Ptr ).

    Subclasses should make sure that they copy parts of, or the whole QuickFixState ,
    which are needed to perform the quick-fix operation.
 */
class TEXTEDITOR_EXPORT QuickFixOperation
{
    Q_DISABLE_COPY(QuickFixOperation)

public:
    typedef QSharedPointer<QuickFixOperation> Ptr;

public:
    QuickFixOperation(int priority = -1);
    virtual ~QuickFixOperation();

    /*!
        \returns The priority for this quick-fix. See the QuickFixCollector for more
                 information.
     */
    virtual int priority() const;

    /// Sets the priority for this quick-fix operation.
    void setPriority(int priority);

    /*!
        \returns The description for this quick-fix. This description is shown to the
                 user.
     */
    virtual QString description() const;

    /// Sets the description for this quick-fix, which will be shown to the user.
    void setDescription(const QString &description);

    /*!
        Perform this quick-fix's operation.

        Subclasses should implement this method to do the actual changes.
     */
    virtual void perform() = 0;

private:
    int _priority;
    QString _description;
};

/*!
    The QuickFixFactory is responsible for generating QuickFixOperation s which are
    applicable to the given QuickFixState.

    A QuickFixFactory should not have any state -- it can be invoked multiple times
    for different QuickFixState objects to create the matching operations, before any
    of those operations are applied (or released).

    This way, a single factory can be used by multiple editors, and a single editor
    can have multiple QuickFixCollector objects for different parts of the code.
 */
class TEXTEDITOR_EXPORT QuickFixFactory: public QObject
{
    Q_OBJECT

public:
    QuickFixFactory(QObject *parent = 0);
    virtual ~QuickFixFactory();

    virtual QList<QuickFixOperation::Ptr>
    matchingOperations(const QSharedPointer<const IAssistInterface> &interface) = 0;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::QuickFixOperation::Ptr)

#endif // TEXTEDITORQUICKFIX_H
