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

#include "Control.h"
#include "Literals.h"
#include "LiteralTable.h"
#include "TranslationUnit.h"
#include "CoreTypes.h"
#include "Symbols.h"
#include "Names.h"
#include "TypeMatcher.h"
#include <map>
#include <set>
#include <algorithm>

using namespace CPlusPlus;

namespace {

template <typename _Tp>
struct Compare;

template <> struct Compare<IntegerType>
{
    bool operator()(const IntegerType &ty, const IntegerType &otherTy) const
    { return ty.kind() < otherTy.kind(); }
};

template <> struct Compare<FloatType>
{
    bool operator()(const FloatType &ty, const FloatType &otherTy) const
    { return ty.kind() < otherTy.kind(); }
};

template <> struct Compare<PointerToMemberType>
{
    bool operator()(const PointerToMemberType &ty, const PointerToMemberType &otherTy) const
    {
        if (ty.memberName() < otherTy.memberName())
            return true;

        else if (ty.memberName() == otherTy.memberName())
            return ty.elementType() < otherTy.elementType();

        return false;
    }
};

template <> struct Compare<PointerType>
{
    bool operator()(const PointerType &ty, const PointerType &otherTy) const
    {
        return ty.elementType() < otherTy.elementType();
    }
};

template <> struct Compare<ReferenceType>
{
    bool operator()(const ReferenceType &ty, const ReferenceType &otherTy) const
    {
        return ty.elementType() < otherTy.elementType();
    }
};

template <> struct Compare<NamedType>
{
    bool operator()(const NamedType &ty, const NamedType &otherTy) const
    {
        return ty.name() < otherTy.name();
    }
};

template <> struct Compare<ArrayType>
{
    bool operator()(const ArrayType &ty, const ArrayType &otherTy) const
    {
        if (ty.size() < otherTy.size())
            return true;

        else if (ty.size() == otherTy.size())
            return ty.elementType() < otherTy.elementType();

        return false;
    }
};

template <> struct Compare<DestructorNameId>
{
    bool operator()(const DestructorNameId &name, const DestructorNameId &otherName) const
    {
        return name.identifier() < otherName.identifier();
    }
};

template <> struct Compare<OperatorNameId>
{
    bool operator()(const OperatorNameId &name, const OperatorNameId &otherName) const
    {
        return name.kind() < otherName.kind();
    }
};

template <> struct Compare<ConversionNameId>
{
    bool operator()(const ConversionNameId &name, const ConversionNameId &otherName) const
    {
        return name.type() < otherName.type();
    }
};
template <> struct Compare<TemplateNameId>
{
    bool operator()(const TemplateNameId &name, const TemplateNameId &otherName) const
    {
        const Identifier *id = name.identifier();
        const Identifier *otherId = otherName.identifier();

        if (id == otherId)
            return std::lexicographical_compare(name.firstTemplateArgument(), name.lastTemplateArgument(),
                                                otherName.firstTemplateArgument(), otherName.lastTemplateArgument());

        return id < otherId;
    }
};
template <> struct Compare<QualifiedNameId>
{
    bool operator()(const QualifiedNameId &name, const QualifiedNameId &otherName) const
    {
        if (name.base() == otherName.base())
            return name.name() < otherName.name();

        return name.base() < otherName.base();
    }
};

template <> struct Compare<SelectorNameId>
{
    bool operator()(const SelectorNameId &name, const SelectorNameId &otherName) const
    {
        if (name.hasArguments() == otherName.hasArguments())
            return std::lexicographical_compare(name.firstName(), name.lastName(),
                                                otherName.firstName(), otherName.lastName());

        return name.hasArguments() < otherName.hasArguments();
    }
};


template <typename _Tp>
class Table: public std::set<_Tp, Compare<_Tp> >
{
    typedef std::set<_Tp, Compare<_Tp> > _Base;
public:
    _Tp *intern(const _Tp &element)
    { return const_cast<_Tp *>(&*_Base::insert(element).first); }
};

} // end of anonymous namespace

#ifdef Q_OS_SYMBIAN
//Symbian compiler has some difficulties to understand the templates.
static void delete_array_entries(std::vector<Symbol *> vt)
{
    std::vector<Symbol *>::iterator it;
    for (it = vt.begin(); it != vt.end(); ++it) {
        delete *it;
    }
}
#else
template <typename _Iterator>
static void delete_array_entries(_Iterator first, _Iterator last)
{
    for (; first != last; ++first)
        delete *first;
}

template <typename _Array>
static void delete_array_entries(const _Array &a)
{ delete_array_entries(a.begin(), a.end()); }
#endif

class Control::Data
{
public:
    Data(Control *control)
        : control(control)
        , translationUnit(0)
        , diagnosticClient(0)
        , deprecatedId(0)
        , unavailableId(0)
        , objcGetterId(0)
        , objcSetterId(0)
        , objcReadwriteId(0)
        , objcReadonlyId(0)
        , objcAssignId(0)
        , objcRetainId(0)
        , objcCopyId(0)
        , objcNonatomicId(0)
        , processor(0)
    {}

    ~Data()
    {
        // symbols
        delete_array_entries(symbols);
    }

    template <typename _Iterator>
    const TemplateNameId *findOrInsertTemplateNameId(const Identifier *id, _Iterator first, _Iterator last)
    {
        return templateNameIds.intern(TemplateNameId(id, first, last));
    }

    const DestructorNameId *findOrInsertDestructorNameId(const Identifier *id)
    {
        return destructorNameIds.intern(DestructorNameId(id));
    }

    const OperatorNameId *findOrInsertOperatorNameId(OperatorNameId::Kind kind)
    {
        return operatorNameIds.intern(OperatorNameId(kind));
    }

    const ConversionNameId *findOrInsertConversionNameId(const FullySpecifiedType &type)
    {
        return conversionNameIds.intern(ConversionNameId(type));
    }

    const QualifiedNameId *findOrInsertQualifiedNameId(const Name *base, const Name *name)
    {
        return qualifiedNameIds.intern(QualifiedNameId(base, name));
    }

    template <typename _Iterator>
    const SelectorNameId *findOrInsertSelectorNameId(_Iterator first, _Iterator last, bool hasArguments)
    {
        return selectorNameIds.intern(SelectorNameId(first, last, hasArguments));
    }

    IntegerType *findOrInsertIntegerType(int kind)
    {
        return integerTypes.intern(IntegerType(kind));
    }

    FloatType *findOrInsertFloatType(int kind)
    {
        return floatTypes.intern(FloatType(kind));
    }

    PointerToMemberType *findOrInsertPointerToMemberType(const Name *memberName, const FullySpecifiedType &elementType)
    {
        return pointerToMemberTypes.intern(PointerToMemberType(memberName, elementType));
    }

    PointerType *findOrInsertPointerType(const FullySpecifiedType &elementType)
    {
        return pointerTypes.intern(PointerType(elementType));
    }

    ReferenceType *findOrInsertReferenceType(const FullySpecifiedType &elementType, bool rvalueRef)
    {
        return referenceTypes.intern(ReferenceType(elementType, rvalueRef));
    }

    ArrayType *findOrInsertArrayType(const FullySpecifiedType &elementType, unsigned size)
    {
        return arrayTypes.intern(ArrayType(elementType, size));
    }

    NamedType *findOrInsertNamedType(const Name *name)
    {
        return namedTypes.intern(NamedType(name));
    }

    Declaration *newDeclaration(unsigned sourceLocation, const Name *name)
    {
        Declaration *declaration = new Declaration(translationUnit, sourceLocation, name);
        symbols.push_back(declaration);
        return declaration;
    }

    Argument *newArgument(unsigned sourceLocation, const Name *name)
    {
        Argument *argument = new Argument(translationUnit, sourceLocation, name);
        symbols.push_back(argument);
        return argument;
    }

    TypenameArgument *newTypenameArgument(unsigned sourceLocation, const Name *name)
    {
        TypenameArgument *argument = new TypenameArgument(translationUnit, sourceLocation, name);
        symbols.push_back(argument);
        return argument;
    }

    Function *newFunction(unsigned sourceLocation, const Name *name)
    {
        Function *function = new Function(translationUnit, sourceLocation, name);
        symbols.push_back(function);
        return function;
    }

    BaseClass *newBaseClass(unsigned sourceLocation, const Name *name)
    {
        BaseClass *baseClass = new BaseClass(translationUnit, sourceLocation, name);
        symbols.push_back(baseClass);
        return baseClass;
    }

    Block *newBlock(unsigned sourceLocation)
    {
        Block *block = new Block(translationUnit, sourceLocation);
        symbols.push_back(block);
        return block;
    }

    Class *newClass(unsigned sourceLocation, const Name *name)
    {
        Class *klass = new Class(translationUnit, sourceLocation, name);
        symbols.push_back(klass);
        return klass;
    }

    Namespace *newNamespace(unsigned sourceLocation, const Name *name)
    {
        Namespace *ns = new Namespace(translationUnit, sourceLocation, name);
        symbols.push_back(ns);
        return ns;
    }

    Template *newTemplate(unsigned sourceLocation, const Name *name)
    {
        Template *ns = new Template(translationUnit, sourceLocation, name);
        symbols.push_back(ns);
        return ns;
    }

    NamespaceAlias *newNamespaceAlias(unsigned sourceLocation, const Name *name)
    {
        NamespaceAlias *ns = new NamespaceAlias(translationUnit, sourceLocation, name);
        symbols.push_back(ns);
        return ns;
    }

    UsingNamespaceDirective *newUsingNamespaceDirective(unsigned sourceLocation, const Name *name)
    {
        UsingNamespaceDirective *u = new UsingNamespaceDirective(translationUnit, sourceLocation, name);
        symbols.push_back(u);
        return u;
    }

    ForwardClassDeclaration *newForwardClassDeclaration(unsigned sourceLocation, const Name *name)
    {
        ForwardClassDeclaration *c = new ForwardClassDeclaration(translationUnit, sourceLocation, name);
        symbols.push_back(c);
        return c;
    }

    QtPropertyDeclaration *newQtPropertyDeclaration(unsigned sourceLocation, const Name *name)
    {
        QtPropertyDeclaration *d = new QtPropertyDeclaration(translationUnit, sourceLocation, name);
        symbols.push_back(d);
        return d;
    }

    QtEnum *newQtEnum(unsigned sourceLocation, const Name *name)
    {
        QtEnum *d = new QtEnum(translationUnit, sourceLocation, name);
        symbols.push_back(d);
        return d;
    }

    ObjCBaseClass *newObjCBaseClass(unsigned sourceLocation, const Name *name)
    {
        ObjCBaseClass *c = new ObjCBaseClass(translationUnit, sourceLocation, name);
        symbols.push_back(c);
        return c;
    }

    ObjCBaseProtocol *newObjCBaseProtocol(unsigned sourceLocation, const Name *name)
    {
        ObjCBaseProtocol *p = new ObjCBaseProtocol(translationUnit, sourceLocation, name);
        symbols.push_back(p);
        return p;
    }

    ObjCClass *newObjCClass(unsigned sourceLocation, const Name *name)
    {
        ObjCClass *c = new ObjCClass(translationUnit, sourceLocation, name);
        symbols.push_back(c);
        return c;
    }

    ObjCForwardClassDeclaration *newObjCForwardClassDeclaration(unsigned sourceLocation, const Name *name)
    {
        ObjCForwardClassDeclaration *fwd = new ObjCForwardClassDeclaration(translationUnit, sourceLocation, name);
        symbols.push_back(fwd);
        return fwd;
    }

    ObjCProtocol *newObjCProtocol(unsigned sourceLocation, const Name *name)
    {
        ObjCProtocol *p = new ObjCProtocol(translationUnit, sourceLocation, name);
        symbols.push_back(p);
        return p;
    }

    ObjCForwardProtocolDeclaration *newObjCForwardProtocolDeclaration(unsigned sourceLocation, const Name *name)
    {
        ObjCForwardProtocolDeclaration *fwd = new ObjCForwardProtocolDeclaration(translationUnit, sourceLocation, name);
        symbols.push_back(fwd);
        return fwd;
    }

    ObjCMethod *newObjCMethod(unsigned sourceLocation, const Name *name)
    {
        ObjCMethod *method = new ObjCMethod(translationUnit, sourceLocation, name);
        symbols.push_back(method);
        return method;
    }

    ObjCPropertyDeclaration *newObjCPropertyDeclaration(unsigned sourceLocation, const Name *name)
    {
        ObjCPropertyDeclaration *decl = new ObjCPropertyDeclaration(translationUnit, sourceLocation, name);
        symbols.push_back(decl);
        return decl;
    }

    Enum *newEnum(unsigned sourceLocation, const Name *name)
    {
        Enum *e = new Enum(translationUnit, sourceLocation, name);
        symbols.push_back(e);
        return e;
    }

    UsingDeclaration *newUsingDeclaration(unsigned sourceLocation, const Name *name)
    {
        UsingDeclaration *u = new UsingDeclaration(translationUnit, sourceLocation, name);
        symbols.push_back(u);
        return u;
    }

    Control *control;
    TranslationUnit *translationUnit;
    DiagnosticClient *diagnosticClient;

    TypeMatcher matcher;

    LiteralTable<Identifier> identifiers;
    LiteralTable<StringLiteral> stringLiterals;
    LiteralTable<NumericLiteral> numericLiterals;

    // ### replace std::map with lookup tables. ASAP!

    // names
    Table<DestructorNameId> destructorNameIds;
    Table<OperatorNameId> operatorNameIds;
    Table<ConversionNameId> conversionNameIds;
    Table<TemplateNameId> templateNameIds;
    Table<QualifiedNameId> qualifiedNameIds;
    Table<SelectorNameId> selectorNameIds;

    // types
    VoidType voidType;
    Table<IntegerType> integerTypes;
    Table<FloatType> floatTypes;
    Table<PointerToMemberType> pointerToMemberTypes;
    Table<PointerType> pointerTypes;
    Table<ReferenceType> referenceTypes;
    Table<ArrayType> arrayTypes;
    Table<NamedType> namedTypes;

    // symbols
    std::vector<Symbol *> symbols;

    const Identifier *deprecatedId;
    const Identifier *unavailableId;
    // ObjC context keywords:
    const Identifier *objcGetterId;
    const Identifier *objcSetterId;
    const Identifier *objcReadwriteId;
    const Identifier *objcReadonlyId;
    const Identifier *objcAssignId;
    const Identifier *objcRetainId;
    const Identifier *objcCopyId;
    const Identifier *objcNonatomicId;
    TopLevelDeclarationProcessor *processor;
};

Control::Control()
{
    d = new Data(this);

    d->deprecatedId = identifier("deprecated");
    d->unavailableId = identifier("unavailable");

    d->objcGetterId = identifier("getter");
    d->objcSetterId = identifier("setter");
    d->objcReadwriteId = identifier("readwrite");
    d->objcReadonlyId = identifier("readonly");
    d->objcAssignId = identifier("assign");
    d->objcRetainId = identifier("retain");
    d->objcCopyId = identifier("copy");
    d->objcNonatomicId = identifier("nonatomic");
}

Control::~Control()
{ delete d; }

TranslationUnit *Control::translationUnit() const
{ return d->translationUnit; }

TranslationUnit *Control::switchTranslationUnit(TranslationUnit *unit)
{
    TranslationUnit *previousTranslationUnit = d->translationUnit;
    d->translationUnit = unit;
    return previousTranslationUnit;
}

DiagnosticClient *Control::diagnosticClient() const
{ return d->diagnosticClient; }

void Control::setDiagnosticClient(DiagnosticClient *diagnosticClient)
{ d->diagnosticClient = diagnosticClient; }

const OperatorNameId *Control::findOperatorNameId(OperatorNameId::Kind operatorId) const
{
    Table<OperatorNameId>::const_iterator i = d->operatorNameIds.find(operatorId);
    if (i == d->operatorNameIds.end())
        return 0;
    else
        return &*i;
}

const Identifier *Control::findIdentifier(const char *chars, unsigned size) const
{ return d->identifiers.findLiteral(chars, size); }

const Identifier *Control::identifier(const char *chars, unsigned size)
{ return d->identifiers.findOrInsertLiteral(chars, size); }

const Identifier *Control::identifier(const char *chars)
{
    unsigned length = std::strlen(chars);
    return identifier(chars, length);
}

Control::IdentifierIterator Control::firstIdentifier() const
{ return d->identifiers.begin(); }

Control::IdentifierIterator Control::lastIdentifier() const
{ return d->identifiers.end(); }

Control::StringLiteralIterator Control::firstStringLiteral() const
{ return d->stringLiterals.begin(); }

Control::StringLiteralIterator Control::lastStringLiteral() const
{ return d->stringLiterals.end(); }

Control::NumericLiteralIterator Control::firstNumericLiteral() const
{ return d->numericLiterals.begin(); }

Control::NumericLiteralIterator Control::lastNumericLiteral() const
{ return d->numericLiterals.end(); }

const StringLiteral *Control::stringLiteral(const char *chars, unsigned size)
{ return d->stringLiterals.findOrInsertLiteral(chars, size); }

const StringLiteral *Control::stringLiteral(const char *chars)
{
    unsigned length = std::strlen(chars);
    return stringLiteral(chars, length);
}

const NumericLiteral *Control::numericLiteral(const char *chars, unsigned size)
{ return d->numericLiterals.findOrInsertLiteral(chars, size); }

const NumericLiteral *Control::numericLiteral(const char *chars)
{
    unsigned length = std::strlen(chars);
    return numericLiteral(chars, length);
}

const TemplateNameId *Control::templateNameId(const Identifier *id,
                                              const FullySpecifiedType *const args,
                                              unsigned argv)
{
    return d->findOrInsertTemplateNameId(id, args, args + argv);
}

const DestructorNameId *Control::destructorNameId(const Identifier *id)
{ return d->findOrInsertDestructorNameId(id); }

const OperatorNameId *Control::operatorNameId(OperatorNameId::Kind kind)
{ return d->findOrInsertOperatorNameId(kind); }

const ConversionNameId *Control::conversionNameId(const FullySpecifiedType &type)
{ return d->findOrInsertConversionNameId(type); }

const QualifiedNameId *Control::qualifiedNameId(const Name *base, const Name *name)
{
    return d->findOrInsertQualifiedNameId(base, name);
}

const SelectorNameId *Control::selectorNameId(const Name *const *names,
                                              unsigned nameCount,
                                              bool hasArguments)
{
    return d->findOrInsertSelectorNameId(names, names + nameCount, hasArguments);
}


VoidType *Control::voidType()
{ return &d->voidType; }

IntegerType *Control::integerType(int kind)
{ return d->findOrInsertIntegerType(kind); }

FloatType *Control::floatType(int kind)
{ return d->findOrInsertFloatType(kind); }

PointerToMemberType *Control::pointerToMemberType(const Name *memberName, const FullySpecifiedType &elementType)
{ return d->findOrInsertPointerToMemberType(memberName, elementType); }

PointerType *Control::pointerType(const FullySpecifiedType &elementType)
{ return d->findOrInsertPointerType(elementType); }

ReferenceType *Control::referenceType(const FullySpecifiedType &elementType, bool rvalueRef)
{ return d->findOrInsertReferenceType(elementType, rvalueRef); }

ArrayType *Control::arrayType(const FullySpecifiedType &elementType, unsigned size)
{ return d->findOrInsertArrayType(elementType, size); }

NamedType *Control::namedType(const Name *name)
{ return d->findOrInsertNamedType(name); }

Argument *Control::newArgument(unsigned sourceLocation, const Name *name)
{ return d->newArgument(sourceLocation, name); }

TypenameArgument *Control::newTypenameArgument(unsigned sourceLocation, const Name *name)
{ return d->newTypenameArgument(sourceLocation, name); }

Function *Control::newFunction(unsigned sourceLocation, const Name *name)
{ return d->newFunction(sourceLocation, name); }

Namespace *Control::newNamespace(unsigned sourceLocation, const Name *name)
{ return d->newNamespace(sourceLocation, name); }

Template *Control::newTemplate(unsigned sourceLocation, const Name *name)
{ return d->newTemplate(sourceLocation, name); }

NamespaceAlias *Control::newNamespaceAlias(unsigned sourceLocation, const Name *name)
{ return d->newNamespaceAlias(sourceLocation, name); }

BaseClass *Control::newBaseClass(unsigned sourceLocation, const Name *name)
{ return d->newBaseClass(sourceLocation, name); }

Class *Control::newClass(unsigned sourceLocation, const Name *name)
{ return d->newClass(sourceLocation, name); }

Enum *Control::newEnum(unsigned sourceLocation, const Name *name)
{ return d->newEnum(sourceLocation, name); }

Block *Control::newBlock(unsigned sourceLocation)
{ return d->newBlock(sourceLocation); }

Declaration *Control::newDeclaration(unsigned sourceLocation, const Name *name)
{ return d->newDeclaration(sourceLocation, name); }

UsingNamespaceDirective *Control::newUsingNamespaceDirective(unsigned sourceLocation,
                                                                const Name *name)
{ return d->newUsingNamespaceDirective(sourceLocation, name); }

UsingDeclaration *Control::newUsingDeclaration(unsigned sourceLocation, const Name *name)
{ return d->newUsingDeclaration(sourceLocation, name); }

ForwardClassDeclaration *Control::newForwardClassDeclaration(unsigned sourceLocation,
                                                             const Name *name)
{ return d->newForwardClassDeclaration(sourceLocation, name); }

QtPropertyDeclaration *Control::newQtPropertyDeclaration(unsigned sourceLocation,
                                                         const Name *name)
{ return d->newQtPropertyDeclaration(sourceLocation, name); }

QtEnum *Control::newQtEnum(unsigned sourceLocation, const Name *name)
{ return d->newQtEnum(sourceLocation, name); }

ObjCBaseClass *Control::newObjCBaseClass(unsigned sourceLocation, const Name *name)
{ return d->newObjCBaseClass(sourceLocation, name); }

ObjCBaseProtocol *Control::newObjCBaseProtocol(unsigned sourceLocation, const Name *name)
{ return d->newObjCBaseProtocol(sourceLocation, name); }

ObjCClass *Control::newObjCClass(unsigned sourceLocation, const Name *name)
{ return d->newObjCClass(sourceLocation, name); }

ObjCForwardClassDeclaration *Control::newObjCForwardClassDeclaration(unsigned sourceLocation, const Name *name)
{ return d->newObjCForwardClassDeclaration(sourceLocation, name); }

ObjCProtocol *Control::newObjCProtocol(unsigned sourceLocation, const Name *name)
{ return d->newObjCProtocol(sourceLocation, name); }

ObjCForwardProtocolDeclaration *Control::newObjCForwardProtocolDeclaration(unsigned sourceLocation, const Name *name)
{ return d->newObjCForwardProtocolDeclaration(sourceLocation, name); }

ObjCMethod *Control::newObjCMethod(unsigned sourceLocation, const Name *name)
{ return d->newObjCMethod(sourceLocation, name); }

ObjCPropertyDeclaration *Control::newObjCPropertyDeclaration(unsigned sourceLocation, const Name *name)
{ return d->newObjCPropertyDeclaration(sourceLocation, name); }

const Identifier *Control::deprecatedId() const
{ return d->deprecatedId; }

const Identifier *Control::unavailableId() const
{ return d->unavailableId; }

const Identifier *Control::objcGetterId() const
{ return d->objcGetterId; }

const Identifier *Control::objcSetterId() const
{ return d->objcSetterId; }

const Identifier *Control::objcReadwriteId() const
{ return d->objcReadwriteId; }

const Identifier *Control::objcReadonlyId() const
{ return d->objcReadonlyId; }

const Identifier *Control::objcAssignId() const
{ return d->objcAssignId; }

const Identifier *Control::objcRetainId() const
{ return d->objcRetainId; }

const Identifier *Control::objcCopyId() const
{ return d->objcCopyId; }

const Identifier *Control::objcNonatomicId() const
{ return d->objcNonatomicId; }

Symbol **Control::firstSymbol() const
{
    if (d->symbols.empty())
        return 0;

    return &*d->symbols.begin();
}

Symbol **Control::lastSymbol() const
{
    if (d->symbols.empty())
        return 0;

    return &*d->symbols.begin() + d->symbols.size();
}

bool Control::hasSymbol(Symbol *symbol) const
{
    return std::find(d->symbols.begin(), d->symbols.end(), symbol) != d->symbols.end();
}

void Control::squeeze()
{
    d->numericLiterals.reset();
}

TopLevelDeclarationProcessor *Control::topLevelDeclarationProcessor() const
{
    return d->processor;
}

void Control::setTopLevelDeclarationProcessor(CPlusPlus::TopLevelDeclarationProcessor *processor)
{
    d->processor = processor;
}
