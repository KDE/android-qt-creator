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

#include <QtTest>
#include <QObject>
#include <QList>
#include <QTextDocument>
#include <QTextBlock>

#include <qmljstools/qmljsqtstylecodeformatter.h>

using namespace QmlJSTools;

class tst_QMLCodeFormatter: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void objectDefinitions1();
    void objectDefinitions2();
    void expressionEndSimple();
    void expressionEnd();
    void expressionEndBracket();
    void expressionEndParen();
    void objectBinding();
    void arrayBinding();
    void functionDeclaration();
    void functionExpression();
    void propertyDeclarations();
    void signalDeclarations();
    void ifBinding1();
    void ifBinding2();
    void ifBinding3();
    void ifStatementWithoutBraces1();
    void ifStatementWithoutBraces2();
    void ifStatementWithoutBraces3();
    void ifStatementWithBraces1();
    void ifStatementWithBraces2();
    void ifStatementWithBraces3();
    void ifStatementMixed();
    void ifStatementAndComments();
    void ifStatementLongCondition();
    void moreIfThenElse();
    void strayElse();
    void oneLineIf();
    void forStatement();
    void whileStatement();
    void tryStatement();
    void doWhile();
    void cStyleComments();
    void cppStyleComments();
    void qmlKeywords();
    void ternary();
    void switch1();
//    void gnuStyle();
//    void whitesmithsStyle();
    void expressionContinuation();
    void objectLiteral1();
    void objectLiteral2();
    void objectLiteral3();
    void objectLiteral4();
    void propertyWithStatement();
    void keywordStatement();
    void namespacedObjects();
    void labelledStatements1();
    void labelledStatements2();
    void labelledStatements3();
};

struct Line {
    Line(QString l)
        : line(l)
    {
        for (int i = 0; i < l.size(); ++i) {
            if (!l.at(i).isSpace()) {
                expectedIndent = i;
                return;
            }
        }
        expectedIndent = l.size();
    }

    Line(QString l, int expect)
        : line(l), expectedIndent(expect)
    {}

    QString line;
    int expectedIndent;
};

QString concatLines(QList<Line> lines)
{
    QString result;
    foreach (const Line &l, lines) {
        result += l.line;
        result += "\n";
    }
    return result;
}

void checkIndent(QList<Line> data, int style = 0)
{
    Q_UNUSED(style)

    QString text = concatLines(data);
    QTextDocument document(text);
    QtStyleCodeFormatter formatter;

    int i = 0;
    foreach (const Line &l, data) {
        QTextBlock b = document.findBlockByLineNumber(i);
        if (l.expectedIndent != -1) {
            int actualIndent = formatter.indentFor(b);
            if (actualIndent != l.expectedIndent) {
                QFAIL(QString("Wrong indent in line %1 with text '%2', expected indent %3, got %4").arg(
                        QString::number(i+1), l.line, QString::number(l.expectedIndent), QString::number(actualIndent)).toLatin1().constData());
            }
        }
        formatter.updateLineStateChange(b);
        ++i;
    }
}

void tst_QMLCodeFormatter::objectDefinitions1()
{
    QList<Line> data;
    data << Line("import Qt 4.7")
         << Line("")
         << Line("Rectangle {")
         << Line("    foo: bar;")
         << Line("    Item {")
         << Line("        x: 42;")
         << Line("        y: x;")
         << Line("    }")
         << Line("    Component.onCompleted: foo;")
         << Line("    ")
         << Line("    Foo.Bar {")
         << Line("        width: 12 + 54;")
         << Line("        anchors.fill: parent;")
         << Line("    }")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::objectDefinitions2()
{
    QList<Line> data;
    data << Line("import Qt 4.7")
         << Line("")
         << Line("Rectangle {")
         << Line("    foo: bar;")
         << Line("    Image { source: \"a+b+c\"; x: 42; y: 12 }")
         << Line("    Component.onCompleted: foo;")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::expressionEndSimple()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    foo: bar +")
         << Line("         foo(4, 5) +")
         << Line("         7")
         << Line("    x: 42")
         << Line("    y: 43")
         << Line("    width: 10")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::expressionEnd()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    foo: bar +")
         << Line("         foo(4")
         << Line("             + 5)")
         << Line("         + 7")
         << Line("    x: 42")
         << Line("       + 43")
         << Line("       + 10")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::expressionEndParen()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    foo: bar")
         << Line("         (foo(4")
         << Line("              + 5)")
         << Line("          + 7,")
         << Line("          abc)")
         << Line("    x: a + b(fpp, ba + 12) + foo(")
         << Line("           bar,")
         << Line("           10)")
         << Line("       + 10")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::expressionEndBracket()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    foo: bar")
         << Line("         [foo[4")
         << Line("              + 5]")
         << Line("          + 7,")
         << Line("          abc]")
         << Line("    x: a + b[fpp, ba + 12] + foo[")
         << Line("           bar,")
         << Line("           10]")
         << Line("       + 10")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::objectBinding()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    foo: bar")
         << Line("    x: 3")
         << Line("    foo: Gradient {")
         << Line("        x: 12")
         << Line("        y: x")
         << Line("    }")
         << Line("    Item {")
         << Line("        states: State {}")
         << Line("    }")
         << Line("    x: 1")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::arrayBinding()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    foo: bar")
         << Line("    x: 3")
         << Line("    foo: [")
         << Line("        State {")
         << Line("            y: x")
         << Line("        },")
         << Line("        State {},")
         << Line("        State")
         << Line("        {")
         << Line("        }")
         << Line("    ]")
         << Line("    foo: [")
         << Line("        1 +")
         << Line("        2")
         << Line("        + 345 * foo(")
         << Line("            bar, car,")
         << Line("            dar),")
         << Line("        x, y,")
         << Line("        z,")
         << Line("    ]")
         << Line("    x: 1")
         << Line("}")
         ;
    checkIndent(data);
}



void tst_QMLCodeFormatter::moreIfThenElse()
{
    QList<Line> data;
    data << Line("Image {")
         << Line("    source: {")
         << Line("        if(type == 1) {")
         << Line("            \"pics/blueStone.png\";")
         << Line("        } else if (type == 2) {")
         << Line("            \"pics/head.png\";")
         << Line("        } else {")
         << Line("            \"pics/redStone.png\";")
         << Line("        }")
         << Line("    }")
         << Line("}");
    checkIndent(data);
}


void tst_QMLCodeFormatter::functionDeclaration()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    foo: bar")
         << Line("    function foo(a, b, c) {")
         << Line("        if (a)")
         << Line("            b;")
         << Line("    }")
         << Line("    property alias boo :")
         << Line("        foo")
         << Line("    Item {")
         << Line("        property variant g : Gradient {")
         << Line("            v: 12")
         << Line("        }")
         << Line("        function bar(")
         << Line("            a, b,")
         << Line("            c)")
         << Line("        {")
         << Line("            var b")
         << Line("        }")
         << Line("        function bar(a,")
         << Line("                     a, b,")
         << Line("                     c)")
         << Line("        {")
         << Line("            var b")
         << Line("        }")
         << Line("    }")
         << Line("    x: 1")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::functionExpression()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("onFoo: {", 4)
         << Line("    function foo(a, b, c) {")
         << Line("        if (a)")
         << Line("            b;")
         << Line("    }")
         << Line("    return function(a, b) { return a + b; }")
         << Line("    return function foo(a, b) {")
         << Line("        return a")
         << Line("    }")
         << Line("}")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::propertyDeclarations()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    foo: bar")
         << Line("    property int foo : 2 +")
         << Line("                       x")
         << Line("    property list<Foo> bar")
         << Line("    property alias boo :")
         << Line("        foo")
         << Line("    Item {")
         << Line("        property variant g : Gradient {")
         << Line("            v: 12")
         << Line("        }")
         << Line("        default property Item g")
         << Line("        default property Item g : parent.foo")
         << Line("    }")
         << Line("    x: 1")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::signalDeclarations()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    foo: bar")
         << Line("    signal foo")
         << Line("    x: bar")
         << Line("    signal bar(a, int b)")
         << Line("    signal bar2()")
         << Line("    Item {")
         << Line("        signal property")
         << Line("        signal import(a, b);")
         << Line("    }")
         << Line("    x: 1")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifBinding1()
{
    QList<Line> data;
    data << Line("A.Rectangle {")
         << Line("    foo: bar")
         << Line("    x: if (a) b")
         << Line("    x: if (a)")
         << Line("           b")
         << Line("    x: if (a) b;")
         << Line("    x: if (a)")
         << Line("           b;")
         << Line("    x: if (a) b; else c")
         << Line("    x: if (a) b")
         << Line("       else c")
         << Line("    x: if (a) b;")
         << Line("       else c")
         << Line("    x: if (a) b;")
         << Line("       else")
         << Line("           c")
         << Line("    x: if (a)")
         << Line("           b")
         << Line("       else")
         << Line("           c")
         << Line("    x: if (a) b; else c;")
         << Line("    x: 1")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifBinding2()
{
    QList<Line> data;
    data << Line("A.Rectangle {")
         << Line("    foo: bar")
         << Line("    x: if (a) b +")
         << Line("              5 +")
         << Line("              5 * foo(")
         << Line("                  1, 2)")
         << Line("       else a =")
         << Line("            foo(15,")
         << Line("                bar(")
         << Line("                    1),")
         << Line("                bar)")
         << Line("    x: if (a) b")
         << Line("              + 5")
         << Line("              + 5")
         << Line("    x: if (a)")
         << Line("           b")
         << Line("                   + 5")
         << Line("                   + 5")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifBinding3()
{
    QList<Line> data;
    data << Line("A.Rectangle {")
         << Line("    foo: bar")
         << Line("    x: if (a) 1")
         << Line("    x: if (a)")
         << Line("           1")
         << Line("    x: if (a) 1;")
         << Line("    x: if (a)")
         << Line("           1;")
         << Line("    x: if (a) 1; else 2")
         << Line("    x: if (a) 1")
         << Line("       else 2")
         << Line("    x: if (a) 1;")
         << Line("       else 2")
         << Line("    x: if (a) 1;")
         << Line("       else")
         << Line("           2")
         << Line("    x: if (a)")
         << Line("           1")
         << Line("       else")
         << Line("           2")
         << Line("    x: if (a) 1; else 2;")
         << Line("    x: 1")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifStatementWithoutBraces1()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    x: if (a)")
         << Line("           if (b)")
         << Line("               foo")
         << Line("           else if (c)")
         << Line("               foo")
         << Line("           else")
         << Line("               if (d)")
         << Line("                   foo;")
         << Line("               else")
         << Line("                   a + b + ")
         << Line("                           c")
         << Line("       else")
         << Line("           foo;")
         << Line("    y: 2")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifStatementWithoutBraces2()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    x: {")
         << Line("        if (a)")
         << Line("            if (b)")
         << Line("                foo;")
         << Line("        if (a) b();")
         << Line("        if (a) b(); else")
         << Line("            foo;")
         << Line("        if (a)")
         << Line("            if (b)")
         << Line("                foo;")
         << Line("            else if (c)")
         << Line("                foo;")
         << Line("            else")
         << Line("                if (d)")
         << Line("                    foo;")
         << Line("                else")
         << Line("                    e")
         << Line("        else")
         << Line("            foo;")
         << Line("    }")
         << Line("    foo: bar")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifStatementWithoutBraces3()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    x: {")
         << Line("        if (a)")
         << Line("            while (b)")
         << Line("                foo;")
         << Line("        while (a) if (a) b();")
         << Line("        if (a) while (a) b; else")
         << Line("            while (c)")
         << Line("                while (d) break")
         << Line("        while (a)")
         << Line("            if (b)")
         << Line("                for (;;) {}")
         << Line("            else if (c)")
         << Line("                for (;;) e")
         << Line("            else")
         << Line("                if (d)")
         << Line("                    foo;")
         << Line("                else")
         << Line("                    e")
         << Line("        if (a) ; else")
         << Line("            while (true)")
         << Line("                f")
         << Line("    }")
         << Line("    foo: bar")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifStatementWithBraces1()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("onClicked: {", 4)
         << Line("    if (a) {")
         << Line("        if (b) {")
         << Line("            foo;")
         << Line("        } else if (c) {")
         << Line("            foo")
         << Line("        } else {")
         << Line("            if (d) {")
         << Line("                foo")
         << Line("            } else {")
         << Line("                foo;")
         << Line("            }")
         << Line("        }")
         << Line("    } else {")
         << Line("        foo;")
         << Line("    }")
         << Line("}")
         << Line("}");
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifStatementWithBraces2()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("onClicked:", 4)
         << Line("    if (a)")
         << Line("    {")
         << Line("        if (b)")
         << Line("        {")
         << Line("            foo")
         << Line("        }")
         << Line("        else if (c)")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         << Line("        else")
         << Line("        {")
         << Line("            if (d)")
         << Line("            {")
         << Line("                foo;")
         << Line("            }")
         << Line("            else")
         << Line("            {")
         << Line("                foo")
         << Line("            }")
         << Line("        }")
         << Line("    }")
         << Line("    else")
         << Line("    {")
         << Line("        foo")
         << Line("    }")
         << Line("}");
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifStatementWithBraces3()
{
    QList<Line> data;
    data << Line("function foo() {")
         << Line("    if (a) {")
         << Line("        continue")
         << Line("    }")
         << Line("    var foo")
         << Line("}");
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifStatementMixed()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("onClicked:", 4)
         << Line("    if (foo)")
         << Line("        if (bar)")
         << Line("        {")
         << Line("            foo;")
         << Line("        }")
         << Line("        else")
         << Line("            if (car)")
         << Line("            {}")
         << Line("            else doo")
         << Line("    else abc")
         << Line("}");
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifStatementAndComments()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("onClicked: {", 4)
         << Line("    if (foo)")
         << Line("        ; // bla")
         << Line("    else if (bar)")
         << Line("        ;")
         << Line("    if (foo)")
         << Line("        ; /*bla")
         << Line("        bla */")
         << Line("    else if (bar)")
         << Line("        // foobar")
         << Line("        ;")
         << Line("    else if (bar)")
         << Line("        /* bla")
         << Line("  bla */")
         << Line("        ;")
         << Line("}")
         << Line("}");
    checkIndent(data);
}

void tst_QMLCodeFormatter::ifStatementLongCondition()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("onClicked: {", 4)
         << Line("    if (foo &&")
         << Line("            bar")
         << Line("            || (a + b > 4")
         << Line("                && foo(bar)")
         << Line("                )")
         << Line("            ) {")
         << Line("        foo;")
         << Line("    }")
         << Line("}");
    checkIndent(data);
}

void tst_QMLCodeFormatter::strayElse()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("onClicked: {", 4)
         << Line("    while ( true ) {}")
         << Line("    else", -1)
         << Line("    else {", -1)
         << Line("    }", -1)
         << Line("}");
    checkIndent(data);
}

void tst_QMLCodeFormatter::oneLineIf()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    onClicked: { if (showIt) show(); }")
         << Line("    x: 2")
         << Line("};")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::forStatement()
{
    QList<Line> data;
    data << Line("for (var i = 0; i < 20; ++i) {")
         << Line("    print(i);")
         << Line("}")
         << Line("for (var x in [a, b, c, d])")
         << Line("    x += 5")
         << Line("var z")
         << Line("for (var x in [a, b, c, d])")
         << Line("    for (;;)")
         << Line("    {")
         << Line("        for (a(); b(); c())")
         << Line("            for (a();")
         << Line("                 b(); c())")
         << Line("                for (a(); b(); c())")
         << Line("                    print(3*d)")
         << Line("    }")
         << Line("z = 2")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::whileStatement()
{
    QList<Line> data;
    data << Line("while (i < 20) {")
         << Line("    print(i);")
         << Line("}")
         << Line("while (x in [a, b, c, d])")
         << Line("    x += 5")
         << Line("var z")
         << Line("while (a + b > 0")
         << Line("       && b + c > 0)")
         << Line("    for (;;)")
         << Line("    {")
         << Line("        for (a(); b(); c())")
         << Line("            while (a())")
         << Line("                for (a(); b(); c())")
         << Line("                    print(3*d)")
         << Line("    }")
         << Line("z = 2")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::tryStatement()
{
    QList<Line> data;
    data << Line("try {")
         << Line("    print(i);")
         << Line("} catch (foo) {")
         << Line("    print(foo)")
         << Line("} finally {")
         << Line("    var z")
         << Line("    while (a + b > 0")
         << Line("           && b + c > 0)")
         << Line("        ;")
         << Line("}")
         << Line("z = 2")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::doWhile()
{
    QList<Line> data;
    data << Line("function foo() {")
         << Line("    do { if (c) foo; } while (a);")
         << Line("    do {")
         << Line("        if (a);")
         << Line("    } while (a);")
         << Line("    do")
         << Line("        foo;")
         << Line("    while (a);")
         << Line("    do foo; while (a);")
         << Line("};")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::cStyleComments()
{
    QList<Line> data;
    data << Line("/*")
         << Line("  ")
         << Line("      foo")
         << Line("      ")
         << Line("   foo")
         << Line("   ")
         << Line("*/")
         << Line("Rectangle {")
         << Line("    /*")
         << Line("      ")
         << Line("   foo")
         << Line("   ")
         << Line("    */")
         << Line("    /* bar */")
         << Line("}")
         << Line("Item {")
         << Line("    /* foo */")
         << Line("    /*")
         << Line("      ")
         << Line("   foo")
         << Line("   ")
         << Line("    */")
         << Line("    /* bar */")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::cppStyleComments()
{
    QList<Line> data;
    data << Line("// abc")
         << Line("Item {  ")
         << Line("    // ghij")
         << Line("    // def")
         << Line("    // ghij")
         << Line("    x: 4 // hik")
         << Line("    // doo")
         << Line("} // ba")
         << Line("// ba")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::ternary()
{
    QList<Line> data;
    data << Line("function foo() {")
         << Line("    var i = a ? b : c;")
         << Line("    foo += a_bigger_condition ?")
         << Line("                b")
         << Line("              : c;")
         << Line("    foo += a_bigger_condition")
         << Line("            ? b")
         << Line("            : c;")
         << Line("    var i = a ?")
         << Line("                b : c;")
         << Line("    var i = aooo ? b")
         << Line("                 : c +")
         << Line("                   2;")
         << Line("    var i = (a ? b : c) + (foo")
         << Line("                           bar);")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::switch1()
{
    QList<Line> data;
    data << Line("function foo() {")
         << Line("    switch (a) {")
         << Line("    case 1:")
         << Line("        foo;")
         << Line("        if (a);")
         << Line("    case 2:")
         << Line("    case 3: {")
         << Line("        foo")
         << Line("    }")
         << Line("    case 4:")
         << Line("    {")
         << Line("        foo;")
         << Line("    }")
         << Line("    case bar:")
         << Line("        break")
         << Line("    }")
         << Line("    case 4:")
         << Line("    {")
         << Line("        if (a) {")
         << Line("        }")
         << Line("    }")
         << Line("}")
         ;
    checkIndent(data);
}

//void tst_QMLCodeFormatter::gnuStyle()
//{
//    QList<Line> data;
//    data << Line("struct S")
//         << Line("{")
//         << Line("    void foo()")
//         << Line("    {")
//         << Line("        if (a)")
//         << Line("            {")
//         << Line("                fpp;")
//         << Line("            }")
//         << Line("        else if (b)")
//         << Line("            {")
//         << Line("                fpp;")
//         << Line("            }")
//         << Line("        else")
//         << Line("            {")
//         << Line("            }")
//         << Line("        if (b) {")
//         << Line("            fpp;")
//         << Line("        }")
//         << Line("    }")
//         << Line("};")
//         ;
//    checkIndent(data, 1);
//}

//void tst_QMLCodeFormatter::whitesmithsStyle()
//{
//    QList<Line> data;
//    data << Line("struct S")
//         << Line("    {")
//         << Line("    void foo()")
//         << Line("        {")
//         << Line("        if (a)")
//         << Line("            {")
//         << Line("            fpp;")
//         << Line("            }")
//         << Line("        if (b) {")
//         << Line("            fpp;")
//         << Line("            }")
//         << Line("        }")
//         << Line("    };")
//         ;
//    checkIndent(data, 2);
//}

void tst_QMLCodeFormatter::qmlKeywords()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    on: 2")
         << Line("    property: 2")
         << Line("    signal: 2")
         << Line("    list: 2")
         << Line("    as: 2")
         << Line("    import: 2")
         << Line("    Item {")
         << Line("    }")
         << Line("    x: 2")
         << Line("};")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::expressionContinuation()
{
    QList<Line> data;
    data << Line("var x = 1 ? 2")
         << Line("            + 3 : 4")
         << Line("++x")
         << Line("++y--")
         << Line("x +=")
         << Line("        y++")
         << Line("var z")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::objectLiteral1()
{
    QList<Line> data;
    data << Line("function shuffle() {")
         << Line("    for (var i = 0; i < 10; ++i) {")
         << Line("        x[i] = { index: i }")
         << Line("    }")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::objectLiteral2()
{
    QList<Line> data;
    data << Line("var x = {")
         << Line("    \"x\": 12,")
         << Line("    \"y\": 34,")
         << Line("    z: \"abc\"")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::objectLiteral3()
{
    QList<Line> data;
    data << Line("var x = {")
         << Line("    x: {")
         << Line("        y: 12,")
         << Line("        z: [1, 3]")
         << Line("    },")
         << Line("    \"z\": {")
         << Line("        a: 1 + 2 + 3,")
         << Line("        b: \"12\"")
         << Line("    },")
         << Line("    a: b")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::objectLiteral4()
{
    QList<Line> data;
    data << Line("var x = { a: 12, b: 13 }")
         << Line("y = {")
         << Line("    a: 1 +")
         << Line("       2 + 3")
         << Line("       + 4")
         << Line("}")
         << Line("y = {")
         << Line("    a: 1 +")
         << Line("       2 + 3")
         << Line("       + 4,")
         << Line("    b: {")
         << Line("        adef: 1 +")
         << Line("              2 + 3")
         << Line("              + 4,")
         << Line("    }")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::propertyWithStatement()
{
    QList<Line> data;
    data << Line("Rectangle {")
         << Line("    property int x: { if (a) break }")
         << Line("    property int y: {")
         << Line("        if (a)")
         << Line("            break")
         << Line("        switch (a) {")
         << Line("        case 1:")
         << Line("        case 2:")
         << Line("            continue")
         << Line("        }")
         << Line("    }")
         << Line("    property int y:")
         << Line("    {")
         << Line("        if (a)")
         << Line("            break")
         << Line("        switch (a) {")
         << Line("        case 1:")
         << Line("        case 2:")
         << Line("            continue")
         << Line("        }")
         << Line("    }")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::keywordStatement()
{
    QList<Line> data;
    data << Line("function shuffle() {")
         << Line("    if (1)")
         << Line("        break")
         << Line("    if (1)")
         << Line("        continue")
         << Line("    if (1)")
         << Line("        throw 1")
         << Line("    if (1)")
         << Line("        return")
         << Line("    if (1)")
         << Line("        return 1")
         << Line("    var x = 2")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::namespacedObjects()
{
    QList<Line> data;
    data << Line("Item {")
         << Line("    width: Q.Foo {")
         << Line("        Q.Bar {")
         << Line("            x: 12")
         << Line("        }")
         << Line("    }")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::labelledStatements1()
{
    QList<Line> data;
    data << Line("lab: while (1) {")
         << Line("    while (1)")
         << Line("        break lab")
         << Line("}")
         << Line("for (;;) {")
         << Line("    lab: do {")
         << Line("        while (1) {")
         << Line("            break lab")
         << Line("        }")
         << Line("    }")
         << Line("}")
         << Line("var x = function() {")
         << Line("    x + 1;")
         << Line("}")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::labelledStatements2()
{
    QList<Line> data;
    data << Line("function a() {")
         << Line("    lab: while (1)")
         << Line("        break lab")
         << Line("    if (a)")
         << Line("        lab: while (1)")
         << Line("            break lab")
         << Line("    var a;")
         << Line("    if (a)")
         << Line("        lab: while (1)")
         << Line("            break lab")
         << Line("    else")
         << Line("        lab: switch (a) {")
         << Line("        case 1:")
         << Line("        }")
         << Line("}")
         << Line("var x")
         ;
    checkIndent(data);
}

void tst_QMLCodeFormatter::labelledStatements3()
{
    QList<Line> data;
    data << Line("function a() {")
         << Line("    lab: while (1)")
         << Line("        break lab")
         << Line("    if (a) {")
         << Line("        lab: while (1)")
         << Line("            break lab")
         << Line("    }")
         << Line("    var a;")
         << Line("    if (a) {")
         << Line("        lab: while (1)")
         << Line("            break lab")
         << Line("    } else {")
         << Line("        lab: switch (a) {")
         << Line("        case 1:")
         << Line("        }")
         << Line("    }")
         << Line("}")
         << Line("var x")
         ;
    checkIndent(data);
}

QTEST_APPLESS_MAIN(tst_QMLCodeFormatter)
#include "tst_qmlcodeformatter.moc"


