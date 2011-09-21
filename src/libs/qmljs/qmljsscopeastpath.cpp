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

#include "qmljsscopeastpath.h"

#include "parser/qmljsast_p.h"

using namespace QmlJS;
using namespace AST;

ScopeAstPath::ScopeAstPath(Document::Ptr doc)
    : _doc(doc)
{
}

QList<Node *> ScopeAstPath::operator()(quint32 offset)
{
    _result.clear();
    _offset = offset;
    if (_doc)
        accept(_doc->ast());
    return _result;
}

void ScopeAstPath::accept(Node *node)
{
    Node::accept(node, this);
}

bool ScopeAstPath::preVisit(Node *node)
{
    if (Statement *stmt = node->statementCast()) {
        return containsOffset(stmt->firstSourceLocation(), stmt->lastSourceLocation());
    } else if (ExpressionNode *exp = node->expressionCast()) {
        return containsOffset(exp->firstSourceLocation(), exp->lastSourceLocation());
    } else if (UiObjectMember *ui = node->uiObjectMemberCast()) {
        return containsOffset(ui->firstSourceLocation(), ui->lastSourceLocation());
    }
    return true;
}

bool ScopeAstPath::visit(UiPublicMember *node)
{
    if (node && node->statement && node->statement->kind == node->Kind_Block
            && containsOffset(node->statement->firstSourceLocation(),
                              node->statement->lastSourceLocation())) {
        _result.append(node);
        accept(node->statement);
        return false;
    }
    return true;
}

bool ScopeAstPath::visit(UiScriptBinding *node)
{
    if (node && node->statement && node->statement->kind == node->Kind_Block
            && containsOffset(node->statement->firstSourceLocation(),
                              node->statement->lastSourceLocation()))
    {
        _result.append(node);
        accept(node->statement);
        return false;
    }
    return true;
}

bool ScopeAstPath::visit(UiObjectDefinition *node)
{
    _result.append(node);
    accept(node->initializer);
    return false;
}

bool ScopeAstPath::visit(UiObjectBinding *node)
{
    _result.append(node);
    accept(node->initializer);
    return false;
}

bool ScopeAstPath::visit(FunctionDeclaration *node)
{
    return visit(static_cast<FunctionExpression *>(node));
}

bool ScopeAstPath::visit(FunctionExpression *node)
{
    accept(node->formals);
    _result.append(node);
    accept(node->body);
    return false;
}

bool ScopeAstPath::containsOffset(SourceLocation start, SourceLocation end)
{
    return _offset >= start.begin() && _offset <= end.end();
}
