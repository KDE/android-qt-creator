/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef GLSL_H
#define GLSL_H

#include <QtCore/qglobal.h>
#include <cstdlib>
#include <cstddef>

#if defined(GLSL_BUILD_LIB)
#  define GLSL_EXPORT Q_DECL_EXPORT
#elif defined(GLSL_BUILD_STATIC_LIB)
#  define GLSL_EXPORT
#else
#  define GLSL_EXPORT Q_DECL_IMPORT
#endif

namespace GLSL {
class Engine;
class Lexer;
class Parser;
class MemoryPool;

// types
class Type;
class UndefinedType;
class VoidType;
class ScalarType;
class BoolType;
class IntType;
class UIntType;
class FloatType;
class DoubleType;
class IndexType;
class VectorType;
class MatrixType;
class ArrayType;
class SamplerType;

// symbols
class Symbol;
class Scope;
class Struct;
class Function;
class Argument;
class Block;
class Variable;
class OverloadSet;
class Namespace;

class AST;
class TranslationUnitAST;
template <typename T> class List;
}

#endif // GLSL_H
