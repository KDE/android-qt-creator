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

#ifndef QMLJSQUICKFIX_H
#define QMLJSQUICKFIX_H

#include "qmljseditor.h"

#include <texteditor/quickfix.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljstools/qmljsrefactoringchanges.h>

#include <QtCore/QSharedPointer>

namespace ExtensionSystem {
class IPlugin;
}

namespace QmlJS {
    class ModelManagerInterface;
}

namespace QmlJSEditor {

namespace Internal {
class QmlJSQuickFixAssistInterface;
} // namespace Internal


/*!
    A quick-fix operation for the QML/JavaScript editor.
 */
class QmlJSQuickFixOperation: public TextEditor::QuickFixOperation
{
public:
    /*!
        Creates a new QmlJSQuickFixOperation.

        \param interface The interface on which the operation is performed.
        \param priority The priority for this operation.
     */
    explicit QmlJSQuickFixOperation(
        const QSharedPointer<const Internal::QmlJSQuickFixAssistInterface> &interface,
        int priority = -1);
    virtual ~QmlJSQuickFixOperation();

    virtual void perform();

protected:
    typedef Utils::ChangeSet::Range Range;

    virtual void performChanges(QmlJSTools::QmlJSRefactoringFilePtr currentFile,
                                const QmlJSTools::QmlJSRefactoringChanges &refactoring) = 0;

    const Internal::QmlJSQuickFixAssistInterface *assistInterface() const;

    /// \returns The name of the file for for which this operation is invoked.
    QString fileName() const;

private:
    QSharedPointer<const Internal::QmlJSQuickFixAssistInterface> m_interface;
};

class QmlJSQuickFixFactory: public TextEditor::QuickFixFactory
{
    Q_OBJECT

public:
    QmlJSQuickFixFactory();
    virtual ~QmlJSQuickFixFactory();

    virtual QList<TextEditor::QuickFixOperation::Ptr>
        matchingOperations(const QSharedPointer<const TextEditor::IAssistInterface> &interface);

    /*!
        Implement this method to match and create the appropriate
        QmlJSQuickFixOperation objects.
     */
    virtual QList<QmlJSQuickFixOperation::Ptr> match(
        const QSharedPointer<const Internal::QmlJSQuickFixAssistInterface> &interface) = 0;

    static QList<QmlJSQuickFixOperation::Ptr> noResult();
    static QList<QmlJSQuickFixOperation::Ptr> singleResult(QmlJSQuickFixOperation *operation);
};

} // namespace QmlJSEditor

#endif // QMLJSQUICKFIX_H
