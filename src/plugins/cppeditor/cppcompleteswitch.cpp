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

#include "cppcompleteswitch.h"

#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cpptools/cpprefactoringchanges.h>
#include <utils/changeset.h>

#include <AST.h>
#include <ASTVisitor.h>
#include <CoreTypes.h>
#include <Symbols.h>

#include <QtGui/QApplication>

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace Utils;

namespace {

class CaseStatementCollector : public ASTVisitor
{
public:
    CaseStatementCollector(Document::Ptr document, const Snapshot &snapshot,
                           Scope *scope)
        : ASTVisitor(document->translationUnit()),
        document(document),
        scope(scope)
    {
        typeOfExpression.init(document, snapshot);
    }

    QStringList operator ()(AST *ast)
    {
        values.clear();
        foundCaseStatementLevel = false;
        accept(ast);
        return values;
    }

    bool preVisit(AST *ast) {
        if (CaseStatementAST *cs = ast->asCaseStatement()) {
            foundCaseStatementLevel = true;
            if (ExpressionAST *expression = cs->expression->asIdExpression()) {
                QList<LookupItem> candidates = typeOfExpression(expression,
                                                                document,
                                                                scope);
                if (!candidates .isEmpty() && candidates.first().declaration()) {
                    Symbol *decl = candidates.first().declaration();
                    values << prettyPrint(LookupContext::fullyQualifiedName(decl));
                }
            }
            return true;
        } else if (foundCaseStatementLevel) {
            return false;
        }
        return true;
    }

    Overview prettyPrint;
    bool foundCaseStatementLevel;
    QStringList values;
    TypeOfExpression typeOfExpression;
    Document::Ptr document;
    Scope *scope;
};

class Operation: public CppQuickFixOperation
{
public:
    Operation(const CppQuickFixState &state, int priority, CompoundStatementAST *compoundStatement, const QStringList &values)
        : CppQuickFixOperation(state, priority)
        , compoundStatement(compoundStatement)
        , values(values)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Complete Switch Statement"));
    }


    virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
    {
        ChangeSet changes;
        int start = currentFile->endOf(compoundStatement->lbrace_token);
        changes.insert(start, QLatin1String("\ncase ")
                       + values.join(QLatin1String(":\nbreak;\ncase "))
                       + QLatin1String(":\nbreak;"));
        currentFile->change(changes);
        currentFile->indent(currentFile->range(compoundStatement));
    }

    CompoundStatementAST *compoundStatement;
    QStringList values;
};

static Enum *findEnum(const QList<LookupItem> &results,
                      const LookupContext &ctxt)
{
    foreach (const LookupItem &result, results) {
        const FullySpecifiedType fst = result.type();

        Type *type = result.declaration() ? result.declaration()->type().type()
                                          : fst.type();

        if (!type)
            continue;
        if (Enum *e = type->asEnumType())
            return e;
        if (const NamedType *namedType = type->asNamedType()) {
            const QList<LookupItem> candidates =
                    ctxt.lookup(namedType->name(), result.scope());
            return findEnum(candidates, ctxt);
        }
    }

    return 0;
}

static Enum *conditionEnum(const CppQuickFixState &state,
                           SwitchStatementAST *statement)
{
    Block *block = statement->symbol;
    Scope *scope = state.document()->scopeAt(block->line(), block->column());
    TypeOfExpression typeOfExpression;
    typeOfExpression.init(state.document(), state.snapshot());
    const QList<LookupItem> results = typeOfExpression(statement->condition,
                                                       state.document(),
                                                       scope);

    return findEnum(results, typeOfExpression.context());
}

} // end of anonymous namespace

QList<CppQuickFixOperation::Ptr> CompleteSwitchCaseStatement::match(const CppQuickFixState &state)
{
    const QList<AST *> &path = state.path();

    if (path.isEmpty())
        return noResult(); // nothing to do

    // look for switch statement
    for (int depth = path.size() - 1; depth >= 0; --depth) {
        AST *ast = path.at(depth);
        SwitchStatementAST *switchStatement = ast->asSwitchStatement();
        if (switchStatement) {
            if (!state.isCursorOn(switchStatement->switch_token) || !switchStatement->statement)
                return noResult();
            CompoundStatementAST *compoundStatement = switchStatement->statement->asCompoundStatement();
            if (!compoundStatement) // we ignore pathologic case "switch (t) case A: ;"
                return noResult();
            // look if the condition's type is an enum
            if (Enum *e = conditionEnum(state, switchStatement)) {
                // check the possible enum values
                QStringList values;
                Overview prettyPrint;
                for (unsigned i = 0; i < e->memberCount(); ++i) {
                    if (Declaration *decl = e->memberAt(i)->asDeclaration()) {
                        values << prettyPrint(LookupContext::fullyQualifiedName(decl));
                    }
                }
                // Get the used values
                Block *block = switchStatement->symbol;
                CaseStatementCollector caseValues(state.document(), state.snapshot(),
                                                  state.document()->scopeAt(block->line(), block->column()));
                QStringList usedValues = caseValues(switchStatement);
                // save the values that would be added
                foreach (const QString &usedValue, usedValues)
                    values.removeAll(usedValue);
                if (values.isEmpty())
                    return noResult();
                else
                    return singleResult(new Operation(state, depth, compoundStatement, values));
            }

            return noResult();
        }
    }

    return noResult();
}
