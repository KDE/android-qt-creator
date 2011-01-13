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

#include "addarraymembervisitor.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

AddArrayMemberVisitor::AddArrayMemberVisitor(QmlDesigner::TextModifier &modifier,
                                             quint32 parentLocation,
                                             const QString &propertyName,
                                             const QString &content):
    QMLRewriter(modifier),
    m_parentLocation(parentLocation),
    m_propertyName(propertyName),
    m_content(content),
    m_convertObjectBindingIntoArrayBinding(false)
{
}

void AddArrayMemberVisitor::findArrayBindingAndInsert(const QString &m_propertyName, UiObjectMemberList *ast)
{
    for (UiObjectMemberList *iter = ast; iter; iter = iter->next) {
        if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(iter->member)) {
            if (flatten(arrayBinding->qualifiedId) == m_propertyName)
                insertInto(arrayBinding);
        } else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(iter->member)) {
            if (flatten(objectBinding->qualifiedId) == m_propertyName && willConvertObjectBindingIntoArrayBinding())
                convertAndAdd(objectBinding);
        }
    }
}

bool AddArrayMemberVisitor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation)
        findArrayBindingAndInsert(m_propertyName, ast->initializer->members);

    return !didRewriting();
}

bool AddArrayMemberVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    if (didRewriting())
        return false;

    if (ast->firstSourceLocation().offset == m_parentLocation)
        findArrayBindingAndInsert(m_propertyName, ast->initializer->members);

    return !didRewriting();
}

// FIXME: duplicate code in the QmlJS::Rewriter class, remove this
void AddArrayMemberVisitor::insertInto(QmlJS::AST::UiArrayBinding *arrayBinding)
{
    UiObjectMember *lastMember = 0;
    for (UiArrayMemberList *iter = arrayBinding->members; iter; iter = iter->next)
        if (iter->member)
            lastMember = iter->member;

    if (!lastMember)
        return; // an array binding cannot be empty, so there will (or should) always be a last member.

    const int insertionPoint = lastMember->lastSourceLocation().end();
    const int indentDepth = calculateIndentDepth(lastMember->firstSourceLocation());

    replace(insertionPoint, 0, QLatin1String(",\n") + addIndentation(m_content, indentDepth));

    setDidRewriting(true);
}

void AddArrayMemberVisitor::convertAndAdd(QmlJS::AST::UiObjectBinding *objectBinding)
{
    const int indentDepth = calculateIndentDepth(objectBinding->firstSourceLocation());
    const QString arrayPrefix = QLatin1String("[\n") + addIndentation(QString(), indentDepth);
    replace(objectBinding->qualifiedTypeNameId->identifierToken.offset, 0, arrayPrefix);
    const int insertionPoint = objectBinding->lastSourceLocation().end();
    replace(insertionPoint, 0,
            QLatin1String(",\n")
            + addIndentation(m_content, indentDepth) + QLatin1Char('\n')
            + addIndentation(QLatin1String("]"), indentDepth)
            );

    setDidRewriting(true);
}
