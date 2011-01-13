/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "removeuiobjectmembervisitor.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <QDebug>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

RemoveUIObjectMemberVisitor::RemoveUIObjectMemberVisitor(QmlDesigner::TextModifier &modifier,
                                                         quint32 objectLocation):
    QMLRewriter(modifier),
    objectLocation(objectLocation)
{
}

bool RemoveUIObjectMemberVisitor::preVisit(Node *ast)
{
    parents.push(ast);

    return true;
}

void RemoveUIObjectMemberVisitor::postVisit(Node *)
{
    parents.pop();
}

bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiPublicMember *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiObjectDefinition *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiSourceElement *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiObjectBinding *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiScriptBinding *ast) { return visitObjectMember(ast); }
bool RemoveUIObjectMemberVisitor::visit(QmlJS::AST::UiArrayBinding *ast) { return visitObjectMember(ast); }

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
bool RemoveUIObjectMemberVisitor::visitObjectMember(QmlJS::AST::UiObjectMember *ast)
{
    const quint32 memberStart = ast->firstSourceLocation().offset;

    if (memberStart == objectLocation) {
        // found it
        int start = objectLocation;
        int end = ast->lastSourceLocation().end();

        if (UiArrayBinding *parentArray = containingArray()) {
            extendToLeadingOrTrailingComma(parentArray, ast, start, end);
        } else {
            includeSurroundingWhitespace(start, end);
        }

        includeLeadingEmptyLine(start);
        replace(start, end - start, QLatin1String(""));

        setDidRewriting(true);

        return false;
    } else if (ast->lastSourceLocation().end() <= objectLocation) {
        // optimization: if the location of the object-to-be-removed is not inside the current member, skip any children
        return false;
    } else {
        // only visit children if the rewriting isn't done yet.
        return !didRewriting();
    }
}

UiArrayBinding *RemoveUIObjectMemberVisitor::containingArray() const
{
    if (parents.size() > 2) {
        if (cast<UiArrayMemberList*>(parents[parents.size() - 2])) {
            return cast<UiArrayBinding*>(parents[parents.size() - 3]);
        }
    }

    return 0;
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void RemoveUIObjectMemberVisitor::extendToLeadingOrTrailingComma(QmlJS::AST::UiArrayBinding *parentArray,
                                                                 QmlJS::AST::UiObjectMember *ast,
                                                                 int &start,
                                                                 int &end) const
{
    UiArrayMemberList *currentMember = 0;
    for (UiArrayMemberList *it = parentArray->members; it; it = it->next) {
        if (it->member == ast) {
            currentMember = it;
            break;
        }
    }

    if (!currentMember)
        return;

    if (currentMember->commaToken.isValid()) {
        // leading comma
        start = currentMember->commaToken.offset;
        if (includeSurroundingWhitespace(start, end))
            --end;
    } else if (currentMember->next && currentMember->next->commaToken.isValid()) {
        // trailing comma
        end = currentMember->next->commaToken.end();
        includeSurroundingWhitespace(start, end);
    } else {
        // array with 1 element, so remove the complete binding
        start = parentArray->firstSourceLocation().offset;
        end = parentArray->lastSourceLocation().end();
        includeSurroundingWhitespace(start, end);
    }
}
