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
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef CPLUSPLUS_PARSER_H
#define CPLUSPLUS_PARSER_H

#include "CPlusPlusForwardDeclarations.h"
#include "ASTfwd.h"
#include "Token.h"
#include "TranslationUnit.h"
#include "MemoryPool.h"
#include <map>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Parser
{
public:
    Parser(TranslationUnit *translationUnit);
    ~Parser();

    bool qtMocRunEnabled() const;
    void setQtMocRunEnabled(bool onoff);

    bool cxx0xEnabled() const;
    void setCxxOxEnabled(bool onoff);

    bool objCEnabled() const;
    void setObjCEnabled(bool onoff);

    bool parseTranslationUnit(TranslationUnitAST *&node);

public:
    bool parseAccessSpecifier(SpecifierAST *&node);
    bool parseExpressionList(ExpressionListAST *&node);
    bool parseAbstractCoreDeclarator(DeclaratorAST *&node, SpecifierListAST *decl_specifier_list);
    bool parseAbstractDeclarator(DeclaratorAST *&node, SpecifierListAST *decl_specifier_list);
    bool parseEmptyDeclaration(DeclarationAST *&node);
    bool parseAccessDeclaration(DeclarationAST *&node);
    bool parseQtPropertyDeclaration(DeclarationAST *&node);
    bool parseQtEnumDeclaration(DeclarationAST *&node);
    bool parseQtFlags(DeclarationAST *&node);
    bool parseQtInterfaces(DeclarationAST *&node);
    bool parseAdditiveExpression(ExpressionAST *&node);
    bool parseAndExpression(ExpressionAST *&node);
    bool parseAsmDefinition(DeclarationAST *&node);
    bool parseAsmOperandList();
    bool parseAsmOperand();
    bool parseAsmClobberList();
    bool parseAssignmentExpression(ExpressionAST *&node);
    bool parseBaseClause(BaseSpecifierListAST *&node);
    bool parseBaseSpecifier(BaseSpecifierListAST *&node);
    bool parseBlockDeclaration(DeclarationAST *&node);
    bool parseCppCastExpression(ExpressionAST *&node);
    bool parseCastExpression(ExpressionAST *&node);
    bool parseClassSpecifier(SpecifierListAST *&node);
    bool parseCommaExpression(ExpressionAST *&node);
    bool parseCompoundStatement(StatementAST *&node);
    bool parseBreakStatement(StatementAST *&node);
    bool parseContinueStatement(StatementAST *&node);
    bool parseGotoStatement(StatementAST *&node);
    bool parseReturnStatement(StatementAST *&node);
    bool parseCondition(ExpressionAST *&node);
    bool parseConditionalExpression(ExpressionAST *&node);
    bool parseConstantExpression(ExpressionAST *&node);
    bool parseCtorInitializer(CtorInitializerAST *&node);
    bool parseCvQualifiers(SpecifierListAST *&node);
    bool parseDeclaratorOrAbstractDeclarator(DeclaratorAST *&node, SpecifierListAST *decl_specifier_list);
    bool parseDeclaration(DeclarationAST *&node);
    bool parseSimpleDeclaration(DeclarationAST *&node, ClassSpecifierAST *declaringClass = 0);
    bool parseDeclarationStatement(StatementAST *&node);
    bool parseCoreDeclarator(DeclaratorAST *&node, SpecifierListAST *decl_specifier_list, ClassSpecifierAST *declaringClass);
    bool parseDeclarator(DeclaratorAST *&node, SpecifierListAST *decl_specifier_list, ClassSpecifierAST *declaringClass = 0);
    bool parseDeleteExpression(ExpressionAST *&node);
    bool parseDoStatement(StatementAST *&node);
    bool parseElaboratedTypeSpecifier(SpecifierListAST *&node);
    bool parseEnumSpecifier(SpecifierListAST *&node);
    bool parseEnumerator(EnumeratorListAST *&node);
    bool parseEqualityExpression(ExpressionAST *&node);
    bool parseExceptionDeclaration(ExceptionDeclarationAST *&node);
    bool parseExceptionSpecification(ExceptionSpecificationAST *&node);
    bool parseExclusiveOrExpression(ExpressionAST *&node);
    bool parseExpression(ExpressionAST *&node);
    bool parseExpressionOrDeclarationStatement(StatementAST *&node);
    bool parseExpressionStatement(StatementAST *&node);
    bool parseForInitStatement(StatementAST *&node);
    bool parseForeachStatement(StatementAST *&node);
    bool parseForStatement(StatementAST *&node);
    bool parseFunctionBody(StatementAST *&node);
    bool parseIfStatement(StatementAST *&node);
    bool parseInclusiveOrExpression(ExpressionAST *&node);
    bool parseInitDeclarator(DeclaratorAST *&node, SpecifierListAST *decl_specifier_list, ClassSpecifierAST *declaringClass);
    bool parseInitializerList(ExpressionListAST *&node);
    bool parseInitializer(ExpressionAST *&node, unsigned *equals_token);
    bool parseInitializerClause(ExpressionAST *&node);
    bool parseLabeledStatement(StatementAST *&node);
    bool parseLinkageBody(DeclarationAST *&node);
    bool parseLinkageSpecification(DeclarationAST *&node);
    bool parseLogicalAndExpression(ExpressionAST *&node);
    bool parseLogicalOrExpression(ExpressionAST *&node);
    bool parseMemInitializer(MemInitializerListAST *&node);
    bool parseMemInitializerList(MemInitializerListAST *&node);
    bool parseMemberSpecification(DeclarationAST *&node, ClassSpecifierAST *declaringClass);
    bool parseMultiplicativeExpression(ExpressionAST *&node);
    bool parseTemplateId(NameAST *&node, unsigned template_token = 0);
    bool parseClassOrNamespaceName(NameAST *&node);
    bool parseNameId(NameAST *&node);
    bool parseName(NameAST *&node, bool acceptTemplateId = true);
    bool parseNestedNameSpecifier(NestedNameSpecifierListAST *&node, bool acceptTemplateId);
    bool parseNestedNameSpecifierOpt(NestedNameSpecifierListAST *&name, bool acceptTemplateId);
    bool parseNamespace(DeclarationAST *&node);
    bool parseNamespaceAliasDefinition(DeclarationAST *&node);
    bool parseNewArrayDeclarator(NewArrayDeclaratorListAST *&node);
    bool parseNewExpression(ExpressionAST *&node);
    bool parseNewPlacement(NewPlacementAST *&node);
    bool parseNewInitializer(NewInitializerAST *&node);
    bool parseNewTypeId(NewTypeIdAST *&node);
    bool parseOperator(OperatorAST *&node);
    bool parseConversionFunctionId(NameAST *&node);
    bool parseOperatorFunctionId(NameAST *&node);
    bool parseParameterDeclaration(ParameterDeclarationAST *&node);
    bool parseParameterDeclarationClause(ParameterDeclarationClauseAST *&node);
    bool parseParameterDeclarationList(ParameterDeclarationListAST *&node);
    bool parsePmExpression(ExpressionAST *&node);
    bool parseTypeidExpression(ExpressionAST *&node);
    bool parseTypenameCallExpression(ExpressionAST *&node);
    bool parseCorePostfixExpression(ExpressionAST *&node);
    bool parsePostfixExpression(ExpressionAST *&node);
    bool parsePostfixExpressionInternal(ExpressionAST *&node);
    bool parsePrimaryExpression(ExpressionAST *&node);
    bool parseNestedExpression(ExpressionAST *&node);
    bool parsePtrOperator(PtrOperatorListAST *&node);
    bool parseRelationalExpression(ExpressionAST *&node);
    bool parseShiftExpression(ExpressionAST *&node);
    bool parseStatement(StatementAST *&node);
    bool parseThisExpression(ExpressionAST *&node);
    bool parseBoolLiteral(ExpressionAST *&node);
    bool parseNumericLiteral(ExpressionAST *&node);
    bool parseStringLiteral(ExpressionAST *&node);
    bool parseSwitchStatement(StatementAST *&node);
    bool parseTemplateArgument(ExpressionAST *&node);
    bool parseTemplateArgumentList(ExpressionListAST *&node);
    bool parseTemplateDeclaration(DeclarationAST *&node);
    bool parseTemplateParameter(DeclarationAST *&node);
    bool parseTemplateParameterList(DeclarationListAST *&node);
    bool parseThrowExpression(ExpressionAST *&node);
    bool parseTryBlockStatement(StatementAST *&node);
    bool parseCatchClause(CatchClauseListAST *&node);
    bool parseTypeId(ExpressionAST *&node);
    bool parseTypeIdList(ExpressionListAST *&node);
    bool parseTypenameTypeParameter(DeclarationAST *&node);
    bool parseTemplateTypeParameter(DeclarationAST *&node);
    bool parseTypeParameter(DeclarationAST *&node);

    bool parseBuiltinTypeSpecifier(SpecifierListAST *&node);
    bool parseAttributeSpecifier(SpecifierListAST *&node);
    bool parseAttributeList(AttributeListAST *&node);

    bool parseSimpleTypeSpecifier(SpecifierListAST *&node)
    { return parseDeclSpecifierSeq(node, true, true); }

    bool parseTypeSpecifier(SpecifierListAST *&node)
    { return parseDeclSpecifierSeq(node, true); }

    bool parseDeclSpecifierSeq(SpecifierListAST *&node,
                               bool onlyTypeSpecifiers = false,
                               bool simplified = false);
    bool parseUnaryExpression(ExpressionAST *&node);
    bool parseUnqualifiedName(NameAST *&node, bool acceptTemplateId = true);
    bool parseUsing(DeclarationAST *&node);
    bool parseUsingDirective(DeclarationAST *&node);
    bool parseWhileStatement(StatementAST *&node);

    void parseExpressionWithOperatorPrecedence(ExpressionAST *&lhs, int minPrecedence);

    // Qt MOC run
    bool parseQtMethod(ExpressionAST *&node);

    // C++0x
    bool parseInitializer0x(ExpressionAST *&node, unsigned *equals_token);
    bool parseBraceOrEqualInitializer0x(ExpressionAST *&node);
    bool parseInitializerClause0x(ExpressionAST *&node);
    bool parseInitializerList0x(ExpressionListAST *&node);
    bool parseBracedInitList0x(ExpressionAST *&node);

    bool parseLambdaExpression(ExpressionAST *&node);
    bool parseLambdaIntroducer(LambdaIntroducerAST *&node);
    bool parseLambdaCapture(LambdaCaptureAST *&node);
    bool parseLambdaDeclarator(LambdaDeclaratorAST *&node);
    bool parseCapture(CaptureAST *&node);
    bool parseCaptureList(CaptureListAST *&node);
    bool parseTrailingReturnType(TrailingReturnTypeAST *&node);
    bool parseTrailingTypeSpecifierSeq(SpecifierListAST *&node);

    // ObjC++
    bool parseObjCExpression(ExpressionAST *&node);
    bool parseObjCClassForwardDeclaration(DeclarationAST *&node);
    bool parseObjCInterface(DeclarationAST *&node,
                            SpecifierListAST *attributes = 0);
    bool parseObjCProtocol(DeclarationAST *&node,
                           SpecifierListAST *attributes = 0);

    bool parseObjCSynchronizedStatement(StatementAST *&node);
    bool parseObjCEncodeExpression(ExpressionAST *&node);
    bool parseObjCProtocolExpression(ExpressionAST *&node);
    bool parseObjCSelectorExpression(ExpressionAST *&node);
    bool parseObjCStringLiteral(ExpressionAST *&node);
    bool parseObjCMessageExpression(ExpressionAST *&node);
    bool parseObjCMessageReceiver(ExpressionAST *&node);
    bool parseObjCMessageArguments(ObjCSelectorAST *&selNode, ObjCMessageArgumentListAST *& argNode);
    bool parseObjCSelectorArg(ObjCSelectorArgumentAST *&selNode, ObjCMessageArgumentAST *&argNode);
    bool parseObjCMethodDefinitionList(DeclarationListAST *&node);
    bool parseObjCMethodDefinition(DeclarationAST *&node);

    bool parseObjCProtocolRefs(ObjCProtocolRefsAST *&node);
    bool parseObjClassInstanceVariables(ObjCInstanceVariablesDeclarationAST *&node);
    bool parseObjCInterfaceMemberDeclaration(DeclarationAST *&node);
    bool parseObjCInstanceVariableDeclaration(DeclarationAST *&node);
    bool parseObjCPropertyDeclaration(DeclarationAST *&node,
                                      SpecifierListAST *attributes = 0);
    bool parseObjCImplementation(DeclarationAST *&node);
    bool parseObjCMethodPrototype(ObjCMethodPrototypeAST *&node);
    bool parseObjCPropertyAttribute(ObjCPropertyAttributeAST *&node);
    bool parseObjCTypeName(ObjCTypeNameAST *&node);
    bool parseObjCSelector(unsigned &selector_token);
    bool parseObjCKeywordDeclaration(ObjCSelectorArgumentAST *&argument, ObjCMessageArgumentDeclarationAST *&node);
    bool parseObjCTypeQualifiers(unsigned &type_qualifier);
    bool peekAtObjCContextKeyword(int kind);
    bool parseObjCContextKeyword(int kind, unsigned &in_token);

    bool lookAtObjCSelector() const;

    bool skipUntil(int token);
    void skipUntilDeclaration();
    bool skipUntilStatement();
    bool skip(int l, int r);

    bool lookAtTypeParameter() const;
    bool lookAtCVQualifier() const;
    bool lookAtFunctionSpecifier() const;
    bool lookAtStorageClassSpecifier() const;
    bool lookAtBuiltinTypeSpecifier() const;
    bool lookAtClassKey() const;

    const Identifier *className(ClassSpecifierAST *ast) const;
    const Identifier *identifier(NameAST *name) const;

    void match(int kind, unsigned *token);

    bool maybeAmbiguousStatement(DeclarationStatementAST *ast, StatementAST *&node);
    bool maybeForwardOrClassDeclaration(SpecifierListAST *decl_specifier_seq) const;

    int peekAtQtContextKeyword() const;

    bool switchTemplateArguments(bool templateArguments);

    bool blockErrors(bool block);
    void warning(unsigned index, const char *format, ...);
    void error(unsigned index, const char *format, ...);
    void fatal(unsigned index, const char *format, ...);

    inline const Token &tok(int i = 1) const
    { return _translationUnit->tokenAt(_tokenIndex + i - 1); }

    inline int LA(int n = 1) const
    { return _translationUnit->tokenKind(_tokenIndex + n - 1); }

    inline int consumeToken()
    { return _tokenIndex++; }

    inline unsigned cursor() const
    { return _tokenIndex; }

    void rewind(unsigned cursor);

    struct TemplateArgumentListEntry {
        unsigned index;
        unsigned cursor;
        ExpressionListAST *ast;

        TemplateArgumentListEntry(unsigned index = 0, unsigned cursor = 0, ExpressionListAST *ast = 0)
            : index(index), cursor(cursor), ast(ast) {}
    };

    TemplateArgumentListEntry *templateArgumentListEntry(unsigned tokenIndex);
    void clearTemplateArgumentList() { _templateArgumentList.clear(); }

private:
    TranslationUnit *_translationUnit;
    Control *_control;
    MemoryPool *_pool;
    unsigned _tokenIndex;
    bool _templateArguments: 1;
    bool _qtMocRunEnabled: 1;
    bool _cxx0xEnabled: 1;
    bool _objCEnabled: 1;
    bool _inFunctionBody: 1;
    bool _inObjCImplementationContext: 1;
    bool _inExpressionStatement: 1;
    int _expressionDepth;

    MemoryPool _expressionStatementTempPool;
    std::map<unsigned, TemplateArgumentListEntry> _templateArgumentList;

    class Rewind;
    friend class Rewind;

private:
    Parser(const Parser& source);
    void operator =(const Parser& source);
};

} // namespace CPlusPlus


#endif // CPLUSPLUS_PARSER_H
