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

#include "Name.h"
#include "Names.h"
#include "NameVisitor.h"

using namespace CPlusPlus;

Name::Name()
{ }

Name::~Name()
{ }

bool Name::isNameId() const
{ return asNameId() != 0; }

bool Name::isTemplateNameId() const
{ return asTemplateNameId() != 0; }

bool Name::isDestructorNameId() const
{ return asDestructorNameId() != 0; }

bool Name::isOperatorNameId() const
{ return asOperatorNameId() != 0; }

bool Name::isConversionNameId() const
{ return asConversionNameId() != 0; }

bool Name::isQualifiedNameId() const
{ return asQualifiedNameId() != 0; }

bool Name::isSelectorNameId() const
{ return asSelectorNameId() != 0; }

void Name::accept(NameVisitor *visitor) const
{
    if (visitor->preVisit(this))
        accept0(visitor);
    visitor->postVisit(this);
}

void Name::accept(const Name *name, NameVisitor *visitor)
{
    if (! name)
        return;
    name->accept(visitor);
}


