/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef QMLJS_STATICANALYSIS_QMLJSSTATICANALYSISMESSAGE_H
#define QMLJS_STATICANALYSIS_QMLJSSTATICANALYSISMESSAGE_H

#include "qmljs_global.h"
#include "parser/qmljsengine_p.h"

#include <QtCore/QRegExp>
#include <QtCore/QString>
#include <QtCore/QList>

namespace QmlJS {
namespace StaticAnalysis {

enum Severity
{
    Hint,         // cosmetic or convention
    MaybeWarning, // possibly a warning, insufficient information
    Warning,      // could cause unintended behavior
    MaybeError,   // possibly an error, insufficient information
    Error         // definitely an error
};

enum Type
{
    // Changing the numbers can break user code.
    // When adding a new check, also add it to the documentation, currently
    // in creator-editors.qdoc.
    UnknownType = 0,
    ErrInvalidEnumValue = 1,
    ErrEnumValueMustBeStringOrNumber = 2,
    ErrNumberValueExpected = 3,
    ErrBooleanValueExpected = 4,
    ErrStringValueExpected = 5,
    ErrInvalidUrl = 6,
    WarnFileOrDirectoryDoesNotExist = 7,
    ErrInvalidColor = 8,
    ErrAnchorLineExpected = 9,
    ErrPropertiesCanOnlyHaveOneBinding = 10,
    ErrIdExpected = 11,
    ErrInvalidId = 14,
    ErrDuplicateId = 15,
    ErrInvalidPropertyName = 16,
    ErrDoesNotHaveMembers = 17,
    ErrInvalidMember = 18,
    WarnAssignmentInCondition = 19,
    WarnCaseWithoutFlowControl = 20,
    WarnEval = 23,
    WarnUnreachable = 28,
    WarnWith = 29,
    WarnComma = 30,
    WarnUnnecessaryMessageSuppression = 31,
    WarnAlreadyFormalParameter = 103,
    WarnAlreadyFunction = 104,
    WarnVarUsedBeforeDeclaration = 105,
    WarnAlreadyVar = 106,
    WarnDuplicateDeclaration = 107,
    WarnFunctionUsedBeforeDeclaration = 108,
    WarnBooleanConstructor = 109,
    WarnStringConstructor = 110,
    WarnObjectConstructor = 111,
    WarnArrayConstructor = 112,
    WarnFunctionConstructor = 113,
    HintAnonymousFunctionSpacing = 114,
    WarnBlock = 115,
    WarnVoid = 116,
    WarnConfusingPluses = 117,
    WarnConfusingMinuses = 119,
    HintDeclareVarsInOneLine = 121,
    HintExtraParentheses = 123,
    MaybeWarnEqualityTypeCoercion = 126,
    WarnConfusingExpressionStatement = 127,
    HintDeclarationsShouldBeAtStartOfFunction = 201,
    HintOneStatementPerLine = 202,
    ErrUnknownComponent = 300,
    ErrCouldNotResolvePrototypeOf = 301,
    ErrCouldNotResolvePrototype = 302,
    ErrPrototypeCycle = 303,
    ErrInvalidPropertyType = 304,
    WarnEqualityTypeCoercion = 305,
    WarnExpectedNewWithUppercaseFunction = 306,
    WarnNewWithLowercaseFunction = 307,
    WarnNumberConstructor = 308,
    HintBinaryOperatorSpacing = 309,
    WarnUnintentinalEmptyBlock = 310,
    HintPreferNonVarPropertyType = 311
};

class QMLJS_EXPORT Message
{
public:
    Message();
    Message(Type type, AST::SourceLocation location,
            const QString &arg1 = QString(),
            const QString &arg2 = QString());

    static QList<Type> allMessageTypes();

    bool isValid() const;
    DiagnosticMessage toDiagnosticMessage() const;

    QString suppressionString() const;
    static QRegExp suppressionPattern();

    AST::SourceLocation location;
    QString message;
    Type type;
    Severity severity;
};

} // namespace StaticAnalysis
} // namespace QmlJS

#endif // QMLJS_STATICANALYSIS_QMLJSSTATICANALYSISMESSAGE_H
