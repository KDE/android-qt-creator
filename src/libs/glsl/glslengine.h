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

#ifndef GLSLENGINE_H
#define GLSLENGINE_H

#include "glsl.h"
#include "glslmemorypool.h"
#include "glsltypes.h"
#include <QtCore/qstring.h>
#include <QtCore/qset.h>
#include <functional>
#include <set>

namespace GLSL {

class GLSL_EXPORT DiagnosticMessage
{
public:
    enum Kind {
        Warning,
        Error
    };

    DiagnosticMessage();

    Kind kind() const;
    void setKind(Kind kind);

    bool isError() const;
    bool isWarning() const;

    QString fileName() const;
    void setFileName(const QString &fileName);

    int line() const;
    void setLine(int line);

    QString message() const;
    void setMessage(const QString &message);

private:
    QString _fileName;
    QString _message;
    Kind _kind;
    int _line;
};

template <typename _Type>
class TypeTable
{
public:
    struct Compare: std::binary_function<_Type, _Type, bool> {
        bool operator()(const _Type &value, const _Type &other) const {
            return value.isLessThan(&other);
        }
    };

    const _Type *intern(const _Type &ty) { return &*_entries.insert(ty).first; }

private:
    std::set<_Type, Compare> _entries;
};

class GLSL_EXPORT Engine
{
public:
    Engine();
    ~Engine();

    const QString *identifier(const QString &s);
    const QString *identifier(const char *s, int n);
    QSet<QString> identifiers() const;

    const QString *number(const QString &s);
    const QString *number(const char *s, int n);
    QSet<QString> numbers() const;

    // types
    const UndefinedType *undefinedType();
    const VoidType *voidType();
    const BoolType *boolType();
    const IntType *intType();
    const UIntType *uintType();
    const FloatType *floatType();
    const DoubleType *doubleType();
    const SamplerType *samplerType(int kind);
    const VectorType *vectorType(const Type *elementType, int dimension);
    const MatrixType *matrixType(const Type *elementType, int columns, int rows);
    const ArrayType *arrayType(const Type *elementType);

    // symbols
    Namespace *newNamespace();
    Struct *newStruct(Scope *scope = 0);
    Block *newBlock(Scope *scope = 0);
    Function *newFunction(Scope *scope = 0);
    Argument *newArgument(Function *function, const QString &name, const Type *type);
    Variable *newVariable(Scope *scope, const QString &name, const Type *type, int qualifiers = 0);

    MemoryPool *pool();

    bool blockDiagnosticMessages(bool block);
    QList<DiagnosticMessage> diagnosticMessages() const;
    void clearDiagnosticMessages();
    void addDiagnosticMessage(const DiagnosticMessage &m);
    void warning(int line, const QString &message);
    void error(int line, const QString &message);

private:
    QSet<QString> _identifiers;
    QSet<QString> _numbers;
    TypeTable<VectorType> _vectorTypes;
    TypeTable<MatrixType> _matrixTypes;
    TypeTable<ArrayType> _arrayTypes;
    TypeTable<SamplerType> _samplerTypes;
    MemoryPool _pool;
    QList<DiagnosticMessage> _diagnosticMessages;
    QList<Symbol *> _symbols;
    bool _blockDiagnosticMessages;
};

} // namespace GLSL

#endif // GLSLENGINE_H
