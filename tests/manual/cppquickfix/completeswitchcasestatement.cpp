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

namespace Foo {
enum Orientation {
    Vertical,
    Horizontal
};
}

class Bar {
    enum AnEnum {
        AValue,
        AnotherValue
    };
};

enum Types {
    TypeA,
    TypeC,
    TypeB,
    TypeD,
    TypeE = TypeD
};

using namespace Foo;

int main()
{
    int j;
    Types t = TypeA;
    Types t2 = TypeB;
    bool b = true;
    enum { foo, bla } i;

    // all handled, don't activate
    switch (t) {
    case TypeA:
    case TypeB:
    case TypeC:
    case TypeD:
    case TypeE:
        ;
    }

    // TypeD must still be added for the outer switch
    switch (t) {
    case TypeA:
        switch (t2) {
        case TypeD: ;
        default: ;
        }
        break;
    case TypeB:
    case TypeE:
        break;
    default:
        ;
    }

    // Resolve type for expressions as switch condition
    switch (b ? t : t2) {
    case TypeA:;
    }

    // Namespaces
    Foo::Orientation o;
    switch (o) {
    case Vertical:
        break;
    }

    // Class members
    Bar::AnEnum a;
    switch (a) {
    case Bar::AnotherValue:
        break;
    }

    // Not a named type
    switch (i) {
    case bla:
        ;
    }

    // ignore pathological case that doesn't have a compound statement
    switch (i)
    case foo: ;
}
