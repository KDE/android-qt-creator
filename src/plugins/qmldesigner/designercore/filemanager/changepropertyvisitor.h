/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CHANGEPROPERTYVISITOR_H
#define CHANGEPROPERTYVISITOR_H

#include "qmlrefactoring.h"
#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class ChangePropertyVisitor: public QMLRewriter
{
public:
    ChangePropertyVisitor(QmlDesigner::TextModifier &modifier,
                          quint32 parentLocation,
                          const QString &name,
                          const QString &value,
                          QmlRefactoring::PropertyType propertyType);

protected:
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);

private:
    void replaceInMembers(QmlJS::AST::UiObjectInitializer *initializer,
                          const QString &propertyName);
    void replaceMemberValue(QmlJS::AST::UiObjectMember *propertyMember, bool needsSemicolon);
    static bool isMatchingPropertyMember(const QString &propName,
                                         QmlJS::AST::UiObjectMember *member);
    static bool nextMemberOnSameLine(QmlJS::AST::UiObjectMemberList *members);

    void insertIntoArray(QmlJS::AST::UiArrayBinding* ast);

private:
    quint32 m_parentLocation;
    QString m_name;
    QString m_value;
    QmlRefactoring::PropertyType m_propertyType;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // CHANGEPROPERTYVISITOR_H
