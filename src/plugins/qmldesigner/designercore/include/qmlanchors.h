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

#ifndef QmlAnchors_H
#define QmlAnchors_H

#include <corelib_global.h>
#include <qmlitemnode.h>


namespace QmlDesigner {

class CORESHARED_EXPORT AnchorLine
{
public:
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

        Fill =  Left | Right | Top | Bottom,
        Center = VerticalCenter | HorizontalCenter,
        HorizontalMask = Left | Right | HorizontalCenter,
        VerticalMask = Top | Bottom | VerticalCenter | Baseline,
        AllMask = VerticalMask | HorizontalMask
    };

    AnchorLine() : m_qmlItemNode(QmlItemNode()), m_type(Invalid) {}
    AnchorLine(const QmlItemNode &fxItemNode, Type type) : m_qmlItemNode(fxItemNode), m_type(type) {}
    Type type() const { return m_type; }
    bool isValid() const { return m_type != Invalid && m_qmlItemNode.isValid(); }

    static bool isHorizontalAnchorLine(Type anchorline);
    static bool isVerticalAnchorLine(Type anchorline);

    QmlItemNode qmlItemNode() const;

private:
    QmlItemNode m_qmlItemNode;
    Type m_type;
};


class CORESHARED_EXPORT QmlAnchors
{
public:
    QmlAnchors(const QmlItemNode &fxItemNode);

    bool isValid() const;

    void setAnchor(AnchorLine::Type sourceAnchorLineType,
                   const QmlItemNode &targetModelNode,
                   AnchorLine::Type targetAnchorLineType);
    bool canAnchor(const QmlItemNode &targetModelNode) const;
    AnchorLine::Type possibleAnchorLines(AnchorLine::Type sourceAnchorLineType,
                                         const QmlItemNode &targetModelNode) const;
    AnchorLine instanceAnchor(AnchorLine::Type sourceAnchorLineType) const;

    void removeAnchor(AnchorLine::Type sourceAnchorLineType);
    void removeAnchors();
    bool instanceHasAnchor(AnchorLine::Type sourceAnchorLineType) const;
    bool instanceHasAnchors() const;
    void setMargin(AnchorLine::Type sourceAnchorLineType, double margin) const;
    bool instanceHasMargin(AnchorLine::Type sourceAnchorLineType) const;
    double instanceMargin(AnchorLine::Type sourceAnchorLineType) const;
    void removeMargin(AnchorLine::Type sourceAnchorLineType);
    void removeMargins();

    void fill();
    void centerIn();

protected:
    QmlItemNode qmlItemNode() const;
    void beautify();

private:
    QmlItemNode m_qmlItemNode;
};

} //QmlDesigner

#endif // QmlAnchors_H
