/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef INTERNALBINDINGPROPERTY_H
#define INTERNALBINDINGPROPERTY_H

#include "internalproperty.h"

namespace QmlDesigner {
namespace Internal {

class InternalBindingProperty : public InternalProperty
{
public:
    typedef QSharedPointer<InternalBindingProperty> Pointer;

    static Pointer create(const QString &name, const InternalNodePointer &propertyOwner);

    bool isValid() const;

    QString expression() const;
    void setExpression(const QString &expression);

    void setDynamicExpression(const QString &type, const QString &expression);


    bool isBindingProperty() const;

protected:
    InternalBindingProperty(const QString &name, const InternalNodePointer &propertyOwner);

private:
    QString m_expression;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // INTERNALBINDINGPROPERTY_H
