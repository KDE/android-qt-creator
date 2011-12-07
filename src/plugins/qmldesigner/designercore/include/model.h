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

#ifndef DESIGNERMODEL_H
#define DESIGNERMODEL_H

#include <corelib_global.h>
#include <QtCore/QObject>
#include <QtCore/QMimeData>
#include <QtCore/QPair>
#include <QtDeclarative/QDeclarativeError>

#include <import.h>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {
    class ModelPrivate;
}

class AnchorLine;
class ModelNode;
class NodeState;
class AbstractView;
class WidgetQueryView;
class NodeStateChangeSet;
class MetaInfo;
class NodeMetaInfo;
class ModelState;
class NodeAnchors;
class AbstractProperty;
class RewriterView;

typedef QList<QPair<QString, QVariant> > PropertyListType;

class CORESHARED_EXPORT Model : public QObject
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::NodeState;
    friend class QmlDesigner::ModelState;
    friend class QmlDesigner::NodeAnchors;
    friend class QmlDesigner::AbstractProperty;
    friend class QmlDesigner::AbstractView;
    friend class Internal::ModelPrivate;

    Q_OBJECT

public:
    enum ViewNotification { NotifyView, DoNotNotifyView };

    virtual ~Model();

    static Model *create(QString type, int major = 4, int minor = 7, Model *metaInfoPropxyModel = 0);

    Model *masterModel() const;
    void setMasterModel(Model *model);

    QUrl fileUrl() const;
    void setFileUrl(const QUrl &url);

    const MetaInfo metaInfo() const;
    MetaInfo metaInfo();
    NodeMetaInfo metaInfo(const QString &typeName, int majorVersion = -1, int minorVersion = -1);
    bool hasNodeMetaInfo(const QString &typeName, int majorVersion = -1, int minorVersion = -1);

    void attachView(AbstractView *view);
    void detachView(AbstractView *view, ViewNotification emitDetachNotify = NotifyView);

    // Editing sub-components:

    // Imports:
    QList<Import> imports() const;
    void changeImports(const QList<Import> &importsToBeAdded, const QList<Import> &importsToBeRemoved);
    bool hasImport(const Import &import, bool ignoreAlias = true, bool allowHigherVersion = false);

    RewriterView *rewriterView() const;

    Model *metaInfoProxyModel();

protected:
    Model();

public:
    Internal::ModelPrivate *d;
};

}

#endif // DESIGNERMODEL_H
