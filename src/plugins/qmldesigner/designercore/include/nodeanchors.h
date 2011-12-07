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

#ifndef NODEANCHORS_H
#define NODEANCHORS_H


#include <QWeakPointer>
#include <QSharedPointer>
#include "corelib_global.h"
#include <anchorline.h>
#include <nodestate.h>

namespace QmlDesigner {
    class Model;
    class NodeState;

    namespace Internal {
        class InternalNode;
        typedef QSharedPointer<InternalNode> InternalNodePointer;
        typedef QWeakPointer<InternalNode> InternalNodeWeakPointer;

        class InternalNodeState;
        typedef QSharedPointer<InternalNodeState> InternalNodeStatePointer;

        class TextToModelMerger;
    }
}

namespace QmlDesigner {

class CORESHARED_EXPORT NodeAnchors
{
    friend class NodeState;
    friend class ModelNode;
    friend class Internal::TextToModelMerger;

public:
    explicit NodeAnchors(const NodeState &nodeState);

    NodeAnchors(const NodeAnchors &other);
    NodeAnchors& operator=(const NodeAnchors &other);
    ~NodeAnchors();

    ModelNode modelNode() const;
    NodeState nodeState() const;

    bool isValid() const;

    void setAnchor(AnchorLine::Type sourceAnchorLineType,
                   const ModelNode &targetModelNode,
                   AnchorLine::Type targetAnchorLineType);
    bool canAnchor(AnchorLine::Type sourceAnchorLineType,
                   const ModelNode &targetModelNode,
                   AnchorLine::Type targetAnchorLineType) const;
    bool canAnchor(const ModelNode &targetModelNode) const;
    AnchorLine::Type possibleAnchorLines(AnchorLine::Type sourceAnchorLineType,
                                         const ModelNode &targetModelNode) const;
    AnchorLine localAnchor(AnchorLine::Type anchorLineType) const;
    AnchorLine anchor(AnchorLine::Type anchorLineType) const;
    void removeAnchor(AnchorLine::Type sourceAnchorLineType);
    void removeAnchors();
    bool hasLocalAnchor(AnchorLine::Type sourceAnchorLineType) const;
    bool hasAnchor(AnchorLine::Type sourceAnchorLineType) const;
    bool hasLocalAnchors() const;
    bool hasAnchors() const;
    void setMargin(AnchorLine::Type sourceAnchorLineType, double margin) const;
    bool hasMargin(AnchorLine::Type sourceAnchorLineType) const;
    double localMargin(AnchorLine::Type sourceAnchorLineType) const;
    double margin(AnchorLine::Type sourceAnchorLineType) const;
    void removeMargin(AnchorLine::Type sourceAnchorLineType);
    void removeMargins();

private: // functions
    NodeAnchors(const Internal::InternalNodeStatePointer &internalNodeState, Model *model);

private: //variables
    Internal::InternalNodePointer m_internalNode;
    Internal::InternalNodeStatePointer m_internalNodeState;
    QWeakPointer<Model> m_model;
};

CORESHARED_EXPORT QDebug operator<<(QDebug debug, const NodeAnchors &anchors);
CORESHARED_EXPORT QTextStream& operator<<(QTextStream &stream, const NodeAnchors &anchors);

} // namespace QmlDesigner

#endif // NODEANCHORS_H
