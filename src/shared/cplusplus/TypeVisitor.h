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

#ifndef CPLUSPLUS_TYPEVISITOR_H
#define CPLUSPLUS_TYPEVISITOR_H

#include "CPlusPlusForwardDeclarations.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT TypeVisitor
{
    TypeVisitor(const TypeVisitor &other);
    void operator =(const TypeVisitor &other);

public:
    TypeVisitor();
    virtual ~TypeVisitor();

    void accept(Type *type);

    virtual bool preVisit(Type *) { return true; }
    virtual void postVisit(Type *) {}

    virtual void visit(UndefinedType *) {}
    virtual void visit(VoidType *) {}
    virtual void visit(IntegerType *) {}
    virtual void visit(FloatType *) {}
    virtual void visit(PointerToMemberType *) {}
    virtual void visit(PointerType *) {}
    virtual void visit(ReferenceType *) {}
    virtual void visit(ArrayType *) {}
    virtual void visit(NamedType *) {}
    virtual void visit(Function *) {}
    virtual void visit(Namespace *) {}
    virtual void visit(Template *) {}
    virtual void visit(Class *) {}
    virtual void visit(Enum *) {}
    virtual void visit(ForwardClassDeclaration *) {}
    virtual void visit(ObjCClass *) {}
    virtual void visit(ObjCProtocol *) {}
    virtual void visit(ObjCMethod *) {}
    virtual void visit(ObjCForwardClassDeclaration *) {}
    virtual void visit(ObjCForwardProtocolDeclaration*) {}
};

} // namespace CPlusPlus


#endif // CPLUSPLUS_TYPEVISITOR_H
