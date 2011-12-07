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

#ifndef QMLJSCHECK_H
#define QMLJSCHECK_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljsscopebuilder.h>
#include <qmljs/qmljsscopechain.h>
#include <qmljs/qmljsstaticanalysismessage.h>
#include <qmljs/parser/qmljsastvisitor_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QSet>
#include <QtCore/QStack>
#include <QtGui/QColor>

namespace QmlJS {

class QMLJS_EXPORT Check: protected AST::Visitor
{
    Q_DECLARE_TR_FUNCTIONS(QmlJS::Check)

    typedef QSet<QString> StringSet;

public:
    // prefer taking root scope chain?
    Check(Document::Ptr doc, const ContextPtr &context);
    virtual ~Check();

    QList<StaticAnalysis::Message> operator()();

    void enableMessage(StaticAnalysis::Type type);
    void disableMessage(StaticAnalysis::Type type);

protected:
    virtual bool preVisit(AST::Node *ast);
    virtual void postVisit(AST::Node *ast);

    virtual bool visit(AST::UiProgram *ast);
    virtual bool visit(AST::UiObjectDefinition *ast);
    virtual bool visit(AST::UiObjectBinding *ast);
    virtual bool visit(AST::UiScriptBinding *ast);
    virtual bool visit(AST::UiArrayBinding *ast);
    virtual bool visit(AST::UiPublicMember *ast);
    virtual bool visit(AST::IdentifierExpression *ast);
    virtual bool visit(AST::FieldMemberExpression *ast);
    virtual bool visit(AST::FunctionDeclaration *ast);
    virtual bool visit(AST::FunctionExpression *ast);
    virtual bool visit(AST::UiObjectInitializer *);

    virtual bool visit(AST::BinaryExpression *ast);
    virtual bool visit(AST::Block *ast);
    virtual bool visit(AST::WithStatement *ast);
    virtual bool visit(AST::VoidExpression *ast);
    virtual bool visit(AST::Expression *ast);
    virtual bool visit(AST::ExpressionStatement *ast);
    virtual bool visit(AST::IfStatement *ast);
    virtual bool visit(AST::ForStatement *ast);
    virtual bool visit(AST::LocalForStatement *ast);
    virtual bool visit(AST::WhileStatement *ast);
    virtual bool visit(AST::DoWhileStatement *ast);
    virtual bool visit(AST::CaseBlock *ast);
    virtual bool visit(AST::NewExpression *ast);
    virtual bool visit(AST::NewMemberExpression *ast);
    virtual bool visit(AST::CallExpression *ast);
    virtual bool visit(AST::StatementList *ast);
    virtual bool visit(AST::ReturnStatement *ast);
    virtual bool visit(AST::ThrowStatement *ast);
    virtual bool visit(AST::DeleteExpression *ast);
    virtual bool visit(AST::TypeOfExpression *ast);

    virtual void endVisit(QmlJS::AST::UiObjectInitializer *);

private:
    void visitQmlObject(AST::Node *ast, AST::UiQualifiedId *typeId,
                        AST::UiObjectInitializer *initializer);
    const Value *checkScopeObjectMember(const AST::UiQualifiedId *id);
    void checkAssignInCondition(AST::ExpressionNode *condition);
    void checkCaseFallthrough(AST::StatementList *statements, AST::SourceLocation errorLoc, AST::SourceLocation nextLoc);
    void checkProperty(QmlJS::AST::UiQualifiedId *);
    void checkNewExpression(AST::ExpressionNode *node);
    void checkBindingRhs(AST::Statement *statement);
    void checkExtraParentheses(AST::ExpressionNode *expression);

    void addMessages(const QList<StaticAnalysis::Message> &messages);
    void addMessage(const StaticAnalysis::Message &message);
    void addMessage(StaticAnalysis::Type type, const AST::SourceLocation &location,
                    const QString &arg1 = QString(), const QString &arg2 = QString());

    void scanCommentsForAnnotations();
    void warnAboutUnnecessarySuppressions();

    AST::Node *parent(int distance = 0);

    Document::Ptr _doc;

    ContextPtr _context;
    ScopeChain _scopeChain;
    ScopeBuilder _scopeBuilder;

    QList<StaticAnalysis::Message> _messages;
    QSet<StaticAnalysis::Type> _enabledMessages;

    const Value *_lastValue;
    QList<AST::Node *> _chain;
    QStack<StringSet> m_idStack;
    QStack<StringSet> m_propertyStack;

    class MessageTypeAndSuppression
    {
    public:
        AST::SourceLocation suppressionSource;
        StaticAnalysis::Type type;
        bool wasSuppressed;
    };

    QHash< int, QList<MessageTypeAndSuppression> > m_disabledMessageTypesByLine;

    bool _importsOk;
    bool _inStatementBinding;
};

} // namespace QmlJS

#endif // QMLJSCHECK_H
