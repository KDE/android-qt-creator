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

#ifndef QMLJSREWRITER_H
#define QMLJSREWRITER_H

#include <qmljs/qmljs_global.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <utils/changeset.h>

#include <QtCore/QStringList>

namespace QmlJS {

class QMLJS_EXPORT Rewriter
{
public:
    enum BindingType {
        ScriptBinding,
        ObjectBinding,
        ArrayBinding
    };

    typedef Utils::ChangeSet::Range Range;

public:
    Rewriter(const QString &originalText,
             Utils::ChangeSet *changeSet,
             const QStringList &propertyOrder);

    Range addBinding(AST::UiObjectInitializer *ast,
                     const QString &propertyName,
                     const QString &propertyValue,
                     BindingType bindingType);

    Range addBinding(AST::UiObjectInitializer *ast,
                     const QString &propertyName,
                     const QString &propertyValue,
                     BindingType bindingType,
                     AST::UiObjectMemberList *insertAfter);

    void changeBinding(AST::UiObjectInitializer *ast,
                       const QString &propertyName,
                       const QString &newValue,
                       BindingType binding);

    void removeBindingByName(AST::UiObjectInitializer *ast, const QString &propertyName);

    void appendToArrayBinding(AST::UiArrayBinding *arrayBinding,
                              const QString &content);

    Range addObject(AST::UiObjectInitializer *ast, const QString &content);
    Range addObject(AST::UiObjectInitializer *ast, const QString &content, AST::UiObjectMemberList *insertAfter);
    Range addObject(AST::UiArrayBinding *ast, const QString &content);
    Range addObject(AST::UiArrayBinding *ast, const QString &content, AST::UiArrayMemberList *insertAfter);

    void removeObjectMember(AST::UiObjectMember *member, AST::UiObjectMember *parent);

    static AST::UiObjectMemberList *searchMemberToInsertAfter(AST::UiObjectMemberList *members, const QStringList &propertyOrder);
    static AST::UiArrayMemberList *searchMemberToInsertAfter(AST::UiArrayMemberList *members, const QStringList &propertyOrder);
    static AST::UiObjectMemberList *searchMemberToInsertAfter(AST::UiObjectMemberList *members, const QString &propertyName, const QStringList &propertyOrder);
    static QString flatten(AST::UiQualifiedId *first);

    static bool includeSurroundingWhitespace(const QString &source, int &start, int &end);
    static void includeLeadingEmptyLine(const QString &source, int &start);
    static void includeEmptyGroupedProperty(AST::UiObjectDefinition *groupedProperty, AST::UiObjectMember *memberToBeRemoved, int &start, int &end);

private:
    void replaceMemberValue(AST::UiObjectMember *propertyMember,
                            const QString &newValue,
                            bool needsSemicolon);
    static bool isMatchingPropertyMember(const QString &propertyName,
                                         AST::UiObjectMember *member);
    static bool nextMemberOnSameLine(AST::UiObjectMemberList *members);

    void insertIntoArray(AST::UiArrayBinding* ast, const QString &newValue);

    void removeMember(AST::UiObjectMember *member);
    void removeGroupedProperty(AST::UiObjectDefinition *ast,
                               const QString &propertyName);

    void extendToLeadingOrTrailingComma(AST::UiArrayBinding *parentArray,
                                        AST::UiObjectMember *member,
                                        int &start,
                                        int &end) const;

private:
    QString m_originalText;
    Utils::ChangeSet *m_changeSet;
    const QStringList m_propertyOrder;
};

} // namespace QmlJS

#endif // QMLJSREWRITER_H
