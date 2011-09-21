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

#ifndef CPPFUNCTIONDECLDEFLINK_H
#define CPPFUNCTIONDECLDEFLINK_H

#include "cppquickfix.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/ASTfwd.h>

#include <cpptools/cpprefactoringchanges.h>
#include <utils/changeset.h>

#include <QtCore/QString>
#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>
#include <QtCore/QFutureWatcher>
#include <QtGui/QTextCursor>

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;
class FunctionDeclDefLink;

class FunctionDeclDefLinkFinder : public QObject
{
    Q_OBJECT
public:
    FunctionDeclDefLinkFinder(QObject *parent = 0);

    void startFindLinkAt(QTextCursor cursor,
                    const CPlusPlus::Document::Ptr &doc,
                    const CPlusPlus::Snapshot &snapshot);

    QTextCursor scannedSelection() const;

signals:
    void foundLink(QSharedPointer<FunctionDeclDefLink> link);

private slots:
    void onFutureDone();

private:
    QTextCursor m_scannedSelection;
    QTextCursor m_nameSelection;
    QFutureWatcher<QSharedPointer<FunctionDeclDefLink> > m_watcher;
};

class FunctionDeclDefLink
{
    Q_DECLARE_TR_FUNCTIONS(FunctionDeclDefLink)
    Q_DISABLE_COPY(FunctionDeclDefLink)
public:
    ~FunctionDeclDefLink();

    class Marker {};

    bool isValid() const;
    bool isMarkerVisible() const;

    void apply(CPPEditorWidget *editor, bool jumpToMatch);
    void hideMarker(CPPEditorWidget *editor);
    void showMarker(CPPEditorWidget *editor);
    Utils::ChangeSet changes(const CPlusPlus::Snapshot &snapshot, int targetOffset = -1);

    QTextCursor linkSelection;

    // stored to allow aborting when the name is changed
    QTextCursor nameSelection;
    QString nameInitial;

    // 1-based line and column
    unsigned targetLine;
    unsigned targetColumn;
    QString targetInitial;

    CPlusPlus::Document::Ptr sourceDocument;
    CPlusPlus::Function *sourceFunction;
    CPlusPlus::DeclarationAST *sourceDeclaration;
    CPlusPlus::FunctionDeclaratorAST *sourceFunctionDeclarator;

    CppTools::CppRefactoringFileConstPtr targetFile;
    CPlusPlus::Function *targetFunction;
    CPlusPlus::DeclarationAST *targetDeclaration;
    CPlusPlus::FunctionDeclaratorAST *targetFunctionDeclarator;

private:
    FunctionDeclDefLink();

    bool hasMarker;

    friend class FunctionDeclDefLinkFinder;
};

class ApplyDeclDefLinkChanges: public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr>
        match(const QSharedPointer<const Internal::CppQuickFixAssistInterface> &interface);
};

} // namespace Internal
} // namespace CppEditor

Q_DECLARE_METATYPE(CppEditor::Internal::FunctionDeclDefLink::Marker)

#endif // CPPFUNCTIONDECLDEFLINK_H
