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

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

#include <QFile>
#include <QList>
#include <QCoreApplication>
#include <QStringList>
#include <QFileInfo>
#include <QTime>
#include <QtDebug>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#ifdef __GNUC__
#  include <cxxabi.h>
#endif

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace std;

class ASTDump: protected Visitor
{
public:
    void operator()(const QString &fileName, const QByteArray &src, Node *ast) {
        _src = src;
        QString basename = fileName;
        int dotIdx = basename.lastIndexOf('.');
        if (dotIdx != -1)
            basename.truncate(dotIdx);
        basename.append(QLatin1String(".ast.dot"));
        out.open(basename.toUtf8().constData());

        out << "digraph AST { ordering=out;" << endl;
        // cout << "rankdir = \"LR\";" << endl;
        Node::accept(ast, this);

        typedef QPair<QByteArray, QByteArray> Pair;

        foreach (const Pair &conn, _connections)
            out << conn.first.constData() << " -> " << conn.second.constData() << endl;

        alignTerminals();

        out << "}" << endl;
        out.close();
        cout << qPrintable(basename) << endl;
    }

protected:
    void alignTerminals() {
        out<<"{ rank=same;" << endl;
        foreach (const QByteArray &terminalShape, _terminalShapes) {
            out << "  " << string(terminalShape) << ";" << endl;
        }
        out<<"}"<<endl;
    }

    static QByteArray name(Node *ast) {
#ifdef __GNUC__
        QByteArray name = abi::__cxa_demangle(typeid(*ast).name(), 0, 0, 0) + 12;
#else
        QByteArray name = typeid(*ast).name();
#endif
        return name;
    }

    QString spell(const SourceLocation &token) {
        return _src.mid(token.offset, token.length).replace('\'', "\\\\").replace('"', "\\\"");
    }

    void terminal(const SourceLocation &token) {
        if (!token.isValid())
            return;

        static int count = 1;
        QByteArray id = 't' + QByteArray::number(count++);
        Node *node = _stack.last();
        _connections.append(qMakePair(_id[node], id));

        QByteArray t;
        t.append(id);
        t.append(" [label = \"");
        t.append(spell(token).toUtf8());
        t.append("\" shape=rect]");
        _terminalShapes.append(t);
    }

    virtual void nonterminal(Node *ast) {
        Node::accept(ast, this);
    }

    virtual void node(Node *ast) {
        out << _id[ast].constData() << " [label=\"" << name(ast).constData() << "\"];" << endl;
    }

    virtual bool preVisit(Node *ast) {
        static int count = 1;
        const QByteArray id = 'n' + QByteArray::number(count++);
        _id[ast] = id;


        if (! _stack.isEmpty())
            _connections.append(qMakePair(_id[_stack.last()], id));

        _stack.append(ast);

        node(ast);

        return true;
    }

    virtual void postVisit(Node *) {
        _stack.removeLast();
    }

protected: // visiting methods:
    virtual bool visit(UiImport *ast) {
        terminal(ast->importToken);

        if (ast->importUri)
            nonterminal(ast->importUri);
        else
            terminal(ast->fileNameToken);

        terminal(ast->versionToken);
        terminal(ast->asToken);
        terminal(ast->importIdToken);
        terminal(ast->semicolonToken);
        return false;
    }

    virtual bool visit(UiObjectBinding *ast) {
        if (ast->hasOnToken) {
            nonterminal(ast->qualifiedTypeNameId);
            terminal(ast->colonToken);
            nonterminal(ast->qualifiedId);
        } else {
            nonterminal(ast->qualifiedId);
            terminal(ast->colonToken);
            nonterminal(ast->qualifiedTypeNameId);
        }
        nonterminal(ast->initializer);
        return false;
    }

    virtual bool visit(UiObjectDefinition *ast) {
        nonterminal(ast->qualifiedTypeNameId);
        nonterminal(ast->initializer);
        return false;
    }

    virtual bool visit(UiObjectInitializer *ast) {
        terminal(ast->lbraceToken);
        nonterminal(ast->members);
        terminal(ast->rbraceToken);
        return false;
    }

    virtual bool visit(UiScriptBinding *ast) {
        nonterminal(ast->qualifiedId);
        terminal(ast->colonToken);
        nonterminal(ast->statement);
        return false;
    }

    virtual bool visit(UiArrayBinding *ast) {
        nonterminal(ast->qualifiedId);
        terminal(ast->colonToken);
        terminal(ast->lbracketToken);
        nonterminal(ast->members);
        terminal(ast->rbracketToken);
        return false;
    }

    virtual bool visit(UiArrayMemberList *ast) {
        terminal(ast->commaToken);
        nonterminal(ast->member);
        nonterminal(ast->next);
        return false;
    }

    virtual bool visit(UiQualifiedId *ast) {
        terminal(ast->identifierToken);
        nonterminal(ast->next);
        return false;
    }

    virtual bool visit(UiPublicMember *ast) {
        // TODO: place the parameters...
//        UiParameterList *parameters;

        terminal(ast->defaultToken);
        terminal(ast->readonlyToken);
        terminal(ast->propertyToken);
        terminal(ast->typeModifierToken);
        terminal(ast->typeToken);
        terminal(ast->identifierToken);
        terminal(ast->colonToken);
        nonterminal(ast->statement);
        nonterminal(ast->binding);
        terminal(ast->semicolonToken);
        return false;
    }

    virtual bool visit(UiFormal *ast) { terminal(ast->identifierToken); terminal(ast->asToken); terminal(ast->aliasToken); return false; }
    virtual bool visit(UiSignature *ast) { terminal(ast->lparenToken); nonterminal(ast->formals); terminal(ast->rparenToken); return false; }

    virtual bool visit(StringLiteral *ast) { terminal(ast->literalToken); return false; }
    virtual bool visit(NumericLiteral *ast) { terminal(ast->literalToken); return false; }
    virtual bool visit(TrueLiteral *ast) { terminal(ast->trueToken); return false; }
    virtual bool visit(FalseLiteral *ast) { terminal(ast->falseToken); return false; }
    virtual bool visit(IdentifierExpression *ast) { terminal(ast->identifierToken); return false; }
    virtual bool visit(FieldMemberExpression *ast) { nonterminal(ast->base); terminal(ast->dotToken); terminal(ast->identifierToken); return false; }
    virtual bool visit(BinaryExpression *ast) { nonterminal(ast->left); terminal(ast->operatorToken); nonterminal(ast->right); return false; }
    virtual bool visit(UnaryPlusExpression *ast) { terminal(ast->plusToken); nonterminal(ast->expression); return false; }
    virtual bool visit(UnaryMinusExpression *ast) { terminal(ast->minusToken); nonterminal(ast->expression); return false; }
    virtual bool visit(NestedExpression *ast) { terminal(ast->lparenToken); nonterminal(ast->expression); terminal(ast->rparenToken); return false; }
    virtual bool visit(ThisExpression *ast) { terminal(ast->thisToken); return false; }
    virtual bool visit(NullExpression *ast) { terminal(ast->nullToken); return false; }
    virtual bool visit(RegExpLiteral *ast) { terminal(ast->literalToken); return false; }
    virtual bool visit(ArrayLiteral *ast) { terminal(ast->lbracketToken); nonterminal(ast->elements); terminal(ast->commaToken); nonterminal(ast->elision); terminal(ast->rbracketToken); return false; }
    virtual bool visit(ObjectLiteral *ast) { terminal(ast->lbraceToken); nonterminal(ast->properties); terminal(ast->rbraceToken); return false; }
    virtual bool visit(ElementList *ast) { nonterminal(ast->next); terminal(ast->commaToken); nonterminal(ast->elision); nonterminal(ast->expression); return false; }
    virtual bool visit(Elision *ast) { nonterminal(ast->next); terminal(ast->commaToken); return false; }
    virtual bool visit(PropertyNameAndValueList *ast) { nonterminal(ast->name); terminal(ast->colonToken); nonterminal(ast->value); terminal(ast->commaToken); nonterminal(ast->next); return false; }
    virtual bool visit(IdentifierPropertyName *ast) { terminal(ast->propertyNameToken); return false; }
    virtual bool visit(StringLiteralPropertyName *ast) { terminal(ast->propertyNameToken); return false; }
    virtual bool visit(NumericLiteralPropertyName *ast) { terminal(ast->propertyNameToken); return false; }
    virtual bool visit(ArrayMemberExpression *ast) { nonterminal(ast->base); terminal(ast->lbracketToken); nonterminal(ast->expression); terminal(ast->rbracketToken); return false; }
    virtual bool visit(NewMemberExpression *ast) { terminal(ast->newToken); nonterminal(ast->base); terminal(ast->lparenToken); nonterminal(ast->arguments); terminal(ast->rparenToken); return false; }
    virtual bool visit(NewExpression *ast) { terminal(ast->newToken); nonterminal(ast->expression); return false; }
    virtual bool visit(CallExpression *ast) { nonterminal(ast->base); terminal(ast->lparenToken); nonterminal(ast->arguments); terminal(ast->rparenToken); return false; }
    virtual bool visit(ArgumentList *ast) { nonterminal(ast->expression); terminal(ast->commaToken); nonterminal(ast->next); return false; }
    virtual bool visit(PostIncrementExpression *ast) { nonterminal(ast->base); terminal(ast->incrementToken); return false; }
    virtual bool visit(PostDecrementExpression *ast) { nonterminal(ast->base); terminal(ast->decrementToken); return false; }
    virtual bool visit(DeleteExpression *ast) { terminal(ast->deleteToken); nonterminal(ast->expression); return false; }
    virtual bool visit(VoidExpression *ast) { terminal(ast->voidToken); nonterminal(ast->expression); return false; }
    virtual bool visit(TypeOfExpression *ast) { terminal(ast->typeofToken); nonterminal(ast->expression); return false; }
    virtual bool visit(PreIncrementExpression *ast) { terminal(ast->incrementToken); nonterminal(ast->expression); return false; }
    virtual bool visit(PreDecrementExpression *ast) { terminal(ast->decrementToken); nonterminal(ast->expression); return false; }
    virtual bool visit(TildeExpression *ast) { terminal(ast->tildeToken); nonterminal(ast->expression); return false; }
    virtual bool visit(NotExpression *ast) { terminal(ast->notToken); nonterminal(ast->expression); return false; }
    virtual bool visit(ConditionalExpression *ast) { nonterminal(ast->expression); terminal(ast->questionToken); nonterminal(ast->ok); terminal(ast->colonToken); nonterminal(ast->ko); return false; }
    virtual bool visit(Expression *ast) { nonterminal(ast->left); terminal(ast->commaToken); nonterminal(ast->right); return false; }
    virtual bool visit(Block *ast) { terminal(ast->lbraceToken); nonterminal(ast->statements); terminal(ast->rbraceToken); return false; }
    virtual bool visit(VariableStatement *ast) { terminal(ast->declarationKindToken); nonterminal(ast->declarations); terminal(ast->semicolonToken); return false; }
    virtual bool visit(VariableDeclaration *ast) { terminal(ast->identifierToken); nonterminal(ast->expression); return false; }
    virtual bool visit(VariableDeclarationList *ast) { nonterminal(ast->declaration); terminal(ast->commaToken); nonterminal(ast->next); return false; }
    virtual bool visit(EmptyStatement* ast) { terminal(ast->semicolonToken); return false; }
    virtual bool visit(ExpressionStatement *ast) { nonterminal(ast->expression); terminal(ast->semicolonToken); return false; }
    virtual bool visit(IfStatement *ast) { terminal(ast->ifToken); terminal(ast->lparenToken); nonterminal(ast->expression); terminal(ast->rparenToken); nonterminal(ast->ok); terminal(ast->elseToken); nonterminal(ast->ko); return false; }
    virtual bool visit(DoWhileStatement *ast) { terminal(ast->doToken); nonterminal(ast->statement); terminal(ast->whileToken); terminal(ast->lparenToken); nonterminal(ast->expression); terminal(ast->rparenToken); terminal(ast->semicolonToken); return false; }

// TODO: visitors for:
//    WhileStatement
//    ForStatement
//    LocalForStatement
//    ForEachStatement
//    LocalForEachStatement
//    ContinueStatement
//    BreakStatement
//    ReturnStatement
//    WithStatement
//    CaseBlock
//    SwitchStatement
//    CaseClause
//    DefaultClause
//    LabelledStatement
//    ThrowStatement
//    Catch
//    Finally
//    TryStatement
//    FunctionExpression
//    FunctionDeclaration
//    DebuggerStatement
//    UiParameterList

private:
    QHash<Node *, QByteArray> _id;
    QList<QPair<QByteArray, QByteArray> > _connections;
    QList<Node *> _stack;
    QList<QByteArray> _terminalShapes;
    ofstream out;
    QByteArray _src;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList files = app.arguments();
    files.removeFirst();

    foreach (const QString &fileName, files) {
        QFile file(fileName);
        if (! file.open(QFile::ReadOnly)) {
            cerr << "Cannot open \"" << qPrintable(fileName)
                      << "\", skipping it." << endl;
            continue;
        }

        const QByteArray source = file.readAll();
        file.close();

        Document::Ptr doc = Document::create(fileName);
        doc->setSource(source);
        doc->parseQml();

        foreach (const DiagnosticMessage &m, doc->diagnosticMessages()) {
            ostream *os;
            if (m.isError()) {
                os = &cerr;
                *os << "Error:";
            } else {
                os = &cout;
                *os << "Warning:";
            }

            if (m.loc.isValid())
                *os << m.loc.startLine << ':' << m.loc.startColumn << ':';
            *os << ' ';
            *os << qPrintable(m.message) << endl;
        }

        ASTDump dump;
        dump(fileName, source, doc->qmlProgram());
    }

    return EXIT_SUCCESS;
}
