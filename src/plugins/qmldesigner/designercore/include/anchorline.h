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

#ifndef ANCHORLINE_H
#define ANCHORLINE_H

#include <QMetaType>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QVariant>

#include "corelib_global.h"

namespace QmlDesigner {

namespace Internal {
    class ModelPrivate;
    class InternalNode;
    typedef QSharedPointer<InternalNode> InternalNodePointer;
    typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;

    class InternalNodeAnchors;

    class InternalNodeState;
    typedef QSharedPointer<InternalNodeState> InternalNodeStatePointer;
    typedef QWeakPointer<InternalNodeState> InternalNodeStateWeakPointer;

    class TextToModelMerger;
}

class NodeAnchors;
class ModelNode;
class NodeState;
class Model;

class CORESHARED_EXPORT AnchorLine
{
public:
    friend class NodeAnchors;
    friend class Internal::InternalNodeAnchors;
    friend class Internal::ModelPrivate;
    friend class Internal::TextToModelMerger;

    enum Type {
        Invalid = 0x0,
        NoAnchor = Invalid,
        Left = 0x01,
        Right = 0x02,
        Top = 0x04,
        Bottom = 0x08,
        HorizontalCenter = 0x10,
        VerticalCenter = 0x20,
        Baseline = 0x40,

        HorizontalMask = Left | Right | HorizontalCenter,
        VerticalMask = Top | Bottom | VerticalCenter | Baseline,
        AllMask = VerticalMask | HorizontalMask
    };

    AnchorLine();
    AnchorLine(const NodeState  &nodeState, Type type);
    ~AnchorLine();
    AnchorLine(const AnchorLine &other);
    AnchorLine &operator =(const AnchorLine &AnchorLine);

    ModelNode modelNode() const;
    Type type() const;
    bool isValid() const;

    QVariant toVariant() const;

protected:
    Internal::InternalNodeStatePointer internalNodeState() const;
    Internal::InternalNodePointer internalNode() const;

private:
    AnchorLine(const Internal::InternalNodeStatePointer &internalNodeState,
               const Internal::InternalNodePointer  &internalNode,
               Model* model,
               Type type);

    Internal::InternalNodeStateWeakPointer m_internalNodeState;
    Internal::InternalNodeWeakPointer m_internalNode;
    QWeakPointer<Model> m_model;
    Type m_anchorType;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::AnchorLine);

#endif // ANCHORLINE_H
