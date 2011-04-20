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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef REMOVEPROPERTYVISITOR_H
#define REMOVEPROPERTYVISITOR_H

#include <QString>

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class RemovePropertyVisitor: public QMLRewriter
{
public:
    RemovePropertyVisitor(QmlDesigner::TextModifier &modifier,
                          quint32 parentLocation,
                          const QString &name);

protected:
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);

private:
    void removeFrom(QmlJS::AST::UiObjectInitializer *ast);
    static bool memberNameMatchesPropertyName(const QString &propertyName,
                                              QmlJS::AST::UiObjectMember *ast);
    void removeGroupedProperty(QmlJS::AST::UiObjectDefinition *ast);
    void removeMember(QmlJS::AST::UiObjectMember *ast);

private:
    quint32 parentLocation;
    QString propertyName;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // REMOVEPROPERTYVISITOR_H
