/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "TypeOfExpression.h"
#include <TranslationUnit.h>
#include "LookupContext.h"
#include "ResolveExpression.h"
#include "pp.h"

#include <AST.h>
#include <Symbol.h>
#include <QSet>

using namespace CPlusPlus;

TypeOfExpression::TypeOfExpression():
    m_ast(0),
    m_scope(0)
{
}

void TypeOfExpression::reset()
{
    m_thisDocument.clear();
    m_snapshot = Snapshot();
    m_ast = 0;
    m_scope = 0;
    m_lookupContext = LookupContext();
    m_bindings.clear();
    m_environment.clear();
}

void TypeOfExpression::init(Document::Ptr thisDocument, const Snapshot &snapshot,
                            QSharedPointer<CreateBindings> bindings)
{
    m_thisDocument = thisDocument;
    m_snapshot = snapshot;
    m_ast = 0;
    m_scope = 0;
    m_lookupContext = LookupContext();
    m_bindings = bindings;
    m_environment.clear();
}

QList<LookupItem> TypeOfExpression::operator()(const QString &expression,
                                               Scope *scope,
                                               PreprocessMode mode)
{
    QString code = expression;

    if (mode == Preprocess)
        code = preprocessedExpression(expression);

    Document::Ptr expressionDoc = documentForExpression(code);
    expressionDoc->check();
    return this->operator ()(extractExpressionAST(expressionDoc),
                             expressionDoc,
                             scope);
}

QList<LookupItem> TypeOfExpression::reference(const QString &expression,
                                              Scope *scope,
                                              PreprocessMode mode)
{
    QString code = expression;

    if (mode == Preprocess)
        code = preprocessedExpression(expression);

    Document::Ptr expressionDoc = documentForExpression(code);
    expressionDoc->check();
    return reference(extractExpressionAST(expressionDoc), expressionDoc, scope);
}

QList<LookupItem> TypeOfExpression::operator()(ExpressionAST *expression,
                                               Document::Ptr document,
                                               Scope *scope)
{
    m_ast = expression;

    m_scope = scope;

    m_lookupContext = LookupContext(document, m_thisDocument, m_snapshot);
    m_lookupContext.setBindings(m_bindings);

    ResolveExpression resolve(m_lookupContext);
    const QList<LookupItem> items = resolve(m_ast, scope);

    if (! m_bindings)
        m_lookupContext = resolve.context();

    return items;
}

QList<LookupItem> TypeOfExpression::reference(ExpressionAST *expression,
                                              Document::Ptr document,
                                              Scope *scope)
{
    m_ast = expression;

    m_scope = scope;

    m_lookupContext = LookupContext(document, m_thisDocument, m_snapshot);
    m_lookupContext.setBindings(m_bindings);

    ResolveExpression resolve(m_lookupContext);
    const QList<LookupItem> items = resolve.reference(m_ast, scope);

    if (! m_bindings)
        m_lookupContext = resolve.context();

    return items;
}

QString TypeOfExpression::preprocess(const QString &expression) const
{
    return preprocessedExpression(expression);
}

ExpressionAST *TypeOfExpression::ast() const
{
    return m_ast;
}

Scope *TypeOfExpression::scope() const
{
    return m_scope;
}

const LookupContext &TypeOfExpression::context() const
{
    return m_lookupContext;
}

ExpressionAST *TypeOfExpression::expressionAST() const
{
    return extractExpressionAST(m_lookupContext.expressionDocument());
}

ExpressionAST *TypeOfExpression::extractExpressionAST(Document::Ptr doc) const
{
    if (! doc->translationUnit()->ast())
        return 0;

    return doc->translationUnit()->ast()->asExpression();
}

Document::Ptr TypeOfExpression::documentForExpression(const QString &expression) const
{
    // create the expression's AST.
    Document::Ptr doc = Document::create(QLatin1String("<completion>"));
    const QByteArray bytes = expression.toUtf8();
    doc->setSource(bytes);
    doc->parse(Document::ParseExpression);
    return doc;
}

void TypeOfExpression::processEnvironment(Document::Ptr doc, Environment *env,
                                          QSet<QString> *processed) const
{
    if (doc && ! processed->contains(doc->fileName())) {
        processed->insert(doc->fileName());

        foreach (const Document::Include &incl, doc->includes())
            processEnvironment(m_snapshot.document(incl.fileName()), env, processed);

        foreach (const Macro &macro, doc->definedMacros())
            env->bind(macro);
    }
}

QString TypeOfExpression::preprocessedExpression(const QString &expression) const
{
    if (expression.trimmed().isEmpty())
        return expression;

    if (! m_environment) {
        Environment *env = new Environment(); // ### cache the environment.

        QSet<QString> processed;
        processEnvironment(m_thisDocument, env, &processed);
        m_environment = QSharedPointer<Environment>(env);
    }

    const QByteArray code = expression.toUtf8();
    Preprocessor preproc(0, m_environment.data());
    const QByteArray preprocessedCode = preproc("<expression>", code);
    return QString::fromUtf8(preprocessedCode.constData(), preprocessedCode.size());
}

