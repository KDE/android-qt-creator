/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef INTERNALPROPERTY_H
#define INTERNALPROPERTY_H

#include "corelib_global.h"

#include <QVariant>
#include <QString>
#include <QRegExp>
#include <QSize>
#include <QSizeF>
#include <QPoint>
#include <QPointF>
#include <QSharedPointer>

namespace QmlDesigner {

namespace Internal {

class InternalBindingProperty;
class InternalVariantProperty;
class InternalNodeListProperty;
class InternalNodeProperty;
class InternalNodeAbstractProperty;
class InternalNode;

typedef QSharedPointer<InternalNode> InternalNodePointer;
typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;

class CORESHARED_EXPORT InternalProperty
{
public:
    typedef QSharedPointer<InternalProperty> Pointer;
    typedef QWeakPointer<InternalProperty> WeakPointer;


    InternalProperty();
    virtual ~InternalProperty();

    virtual bool isValid() const;

    QString name() const;

    virtual bool isBindingProperty() const;
    virtual bool isVariantProperty() const;
    virtual bool isNodeListProperty() const;
    virtual bool isNodeProperty() const;
    virtual bool isNodeAbstractProperty() const;

    QSharedPointer<InternalBindingProperty> toBindingProperty() const;
    QSharedPointer<InternalVariantProperty> toVariantProperty() const;
    QSharedPointer<InternalNodeListProperty> toNodeListProperty() const;
    QSharedPointer<InternalNodeProperty> toNodeProperty() const;
    QSharedPointer<InternalNodeAbstractProperty> toNodeAbstractProperty() const;

    InternalNodePointer propertyOwner() const;

    virtual void remove();


    QString dynamicTypeName() const;

    void resetDynamicTypeName();

protected: // functions
    InternalProperty(const QString &name, const InternalNodePointer &propertyOwner);
    Pointer internalPointer() const;
    void setInternalWeakPointer(const Pointer &pointer);
    void setDynamicTypeName(const QString &name);
private:
    WeakPointer m_internalPointer;
    QString m_name;
    QString m_dynamicType;
    InternalNodeWeakPointer m_propertyOwner;

};

} // namespace Internal
} // namespace QmlDesigner

#endif // INTERNALPROPERTY_H
