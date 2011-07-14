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

#ifndef QMLJSSCOPEASTPATH_H
#define QMLJSSCOPEASTPATH_H

#include "qmljs_global.h"
#include "parser/qmljsastvisitor_p.h"
#include "qmljsdocument.h"

namespace QmlJS {

class QMLJS_EXPORT ScopeAstPath: protected AST::Visitor
{
public:
    ScopeAstPath(Document::Ptr doc);

    QList<AST::Node *> operator()(quint32 offset);

protected:
    void accept(AST::Node *node);

    using Visitor::visit;

    virtual bool preVisit(AST::Node *node);
    virtual bool visit(AST::UiPublicMember *node);
    virtual bool visit(AST::UiScriptBinding *node);
    virtual bool visit(AST::UiObjectDefinition *node);
    virtual bool visit(AST::UiObjectBinding *node);
    virtual bool visit(AST::FunctionDeclaration *node);
    virtual bool visit(AST::FunctionExpression *node);

private:
    bool containsOffset(AST::SourceLocation start, AST::SourceLocation end);

    QList<AST::Node *> _result;
    Document::Ptr _doc;
    quint32 _offset;
};

} // namespace QmlJS

#endif // QMLJSSCOPEASTPATH_H
