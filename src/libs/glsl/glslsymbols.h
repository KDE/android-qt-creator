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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GLSLSYMBOLS_H
#define GLSLSYMBOLS_H

#include "glsltype.h"
#include "glslsymbol.h"
#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QHash>

namespace GLSL {

class GLSL_EXPORT Argument: public Symbol
{
public:
    Argument(Function *scope);

    virtual const Type *type() const;
    void setType(const Type *type);

    virtual Argument *asArgument() { return this; }

private:
    const Type *_type;
};

class GLSL_EXPORT Variable: public Symbol
{
public:
    Variable(Scope *scope);

    virtual const Type *type() const;
    void setType(const Type *type);

    int qualifiers() const { return _qualifiers; }
    void setQualifiers(int qualifiers) { _qualifiers = qualifiers; }

    virtual Variable *asVariable() { return this; }

private:
    const Type *_type;
    int _qualifiers;
};

class GLSL_EXPORT Block: public Scope
{
public:
    Block(Scope *enclosingScope = 0);

    virtual QList<Symbol *> members() const;
    virtual void add(Symbol *symbol);

    virtual Block *asBlock() { return this; }

    virtual const Type *type() const;
    virtual Symbol *find(const QString &name) const;

private:
    QHash<QString, Symbol *> _members;
};

class GLSL_EXPORT Namespace: public Scope
{
public:
    Namespace();
    virtual ~Namespace();

    void add(Symbol *symbol);

    virtual Namespace *asNamespace() { return this; }

    virtual QList<Symbol *> members() const;
    virtual const Type *type() const;
    virtual Symbol *find(const QString &name) const;

private:
    QHash<QString, Symbol *> _members;
    QVector<OverloadSet *> _overloadSets;
};

} // namespace GLSL

#endif // GLSLSYMBOLS_H
