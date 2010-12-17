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

#ifndef MODELREWRITER_H
#define MODELREWRITER_H

#include <QtCore/QByteArray>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QUrl>

#include "anchorline.h"
#include "import.h"
#include "modeltotextmerger.h"
#include "model.h"
#include "modelnode.h"
#include "modificationgrouptoken.h"
#include "subcomponentmanager.h"
#include "texttomodelmerger.h"

namespace QmlDesigner {
class TextModifier;

namespace Internal {

class ModelPrivate;

class ModelRewriter: public QObject
{
    Q_OBJECT

public:
    ModelRewriter(Model* model, TextModifier* textModifier);

    TextModifier *textModifier() const { return m_textModifier; }

    bool load(const QUrl &url, QList<QmlError> *errors = 0);
    bool reloadAndMerge(QList<QmlError> *errors = 0);
    void setFileUrl(const QUrl& url);

    ModificationGroupToken beginModificationGroup();
    void endModificationGroup(const ModificationGroupToken& token);

    // Property change actions:
    void addProperty(const InternalNodeState::Pointer& state, const QString& name, const QVariant& value);
    void changePropertyValue(const InternalNodeState::Pointer& state, const QString& name, const QVariant& value);
    void removeProperty(const InternalNodeState::Pointer& state, const QString& name);

    // Node change actions:
    void addNode(const InternalNodePointer& newNode, const InternalNodePointer& parentNode);
    void reparentNode(const InternalNodePointer &child, const InternalNodePointer &newParent);
    void removeNode(const InternalNodePointer &node);
    void changeNodeId(const InternalNodePointer& node, const QString& id);
    void moveNodeBefore(const InternalNodePointer& node, const InternalNodePointer& beforeNode);

    // State change actions:
    void addModelState(const InternalModelState::Pointer &state);
    void removeModelState(const InternalModelState::Pointer &state);
    void setStateName(const InternalModelStatePointer &state, const QString &name);
    void setStateWhenCondition(const InternalModelStatePointer &state, const QString &whenCondition);

    // Anchors:
    void setAnchor(const InternalNodeState::Pointer &state, const QString &propertyName, const QVariant &value);
    void removeAnchor(const InternalNodeState::Pointer &state, const QString &propertyName);
    void setAnchorMargin(const InternalNodeState::Pointer &state, const QString &propertyName, const QVariant &value);

    bool lastRewriteFailed() const;

    // Copy & Paste:
    QMimeData* copy(const QList<InternalNodeState::Pointer> &nodeStates) const;
    bool paste(QMimeData *transferData, const InternalNode::Pointer &intoNode);

    // Imports:
    void addImport(const Import &import);
    void removeImport(const Import &import);

public slots:
    void mergeChanges();

signals:
    void errorDuringRewrite(const QList<QmlError> &errors);

private:
    bool modificationGroupActive() const { return !m_activeModificationGroups.isEmpty(); }

private:
    SubComponentManager m_subcomponentManager;
    QByteArray m_lastCorrectData;
    QList<ModificationGroupToken> m_activeModificationGroups;
    TextModifier* m_textModifier;
    ModelToTextMerger m_modelToTextMerger;
    TextToModelMerger m_textToModelMerger;
    bool m_lastRewriteFailed;
};

}
}

#endif //MODELREWRITER_H
