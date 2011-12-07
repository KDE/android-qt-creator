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

#ifndef FINDDERIVEDCLASSES_H
#define FINDDERIVEDCLASSES_H

#include "CppDocument.h"
#include "ModelManagerInterface.h"
#include "Overview.h"

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QSet>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT TypeHierarchy
{
    friend class TypeHierarchyBuilder;

public:
    TypeHierarchy();
    TypeHierarchy(Symbol *symbol);

    Symbol *symbol() const;
    const QList<TypeHierarchy> &hierarchy() const;

private:
    Symbol *_symbol;
    QList<TypeHierarchy> _hierarchy;
};

class CPLUSPLUS_EXPORT TypeHierarchyBuilder
{
public:
    TypeHierarchyBuilder(Symbol *symbol, const Snapshot &snapshot);

    TypeHierarchy buildDerivedTypeHierarchy();

private:
    void reset();
    void buildDerived(TypeHierarchy *typeHierarchy);

    Symbol *_symbol;
    Snapshot _snapshot;
    QStringList _dependencies;
    QSet<Symbol *> _visited;
    QHash<QString, QSet<QString> > _candidates;
    Overview _overview;
};

} // CPlusPlus

#endif // FINDDERIVEDCLASSES_H
