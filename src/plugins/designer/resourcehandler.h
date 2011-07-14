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

#ifndef RESOURCEHANDLER_H
#define RESOURCEHANDLER_H

#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
#if QT_VERSION >= 0x050000
class QDesignerFormWindowInterface;
#else
namespace qdesigner_internal {
    class FormWindowBase;
}
#endif
QT_END_NAMESPACE

namespace ProjectExplorer {
class SessionNode;
class NodesWatcher;
}

namespace Designer {
namespace Internal {

/* ResourceHandler: Constructed on a form window and activated on open/save as
 * (see README.txt). The form can have 2 states:
 * 1) standalone: Uses the form editor's list of resource files.
 * 2) Within a project: Use the list of resources files of the projects.
 *
 * When initializing, store the original list of qrc files of the form and
 * connect to various signals of the project explorer to re-check.
 * In updateResources, check whether the form is part of a project and use
 * the project's resource files or the stored ones. */

class ResourceHandler : public QObject
{
    Q_OBJECT
public:
#if QT_VERSION >= 0x050000
        explicit ResourceHandler(QDesignerFormWindowInterface *fw);
#else
        explicit ResourceHandler(qdesigner_internal::FormWindowBase *fw);
#endif
    virtual ~ResourceHandler();

public slots:
    void updateResources();

private:
    void ensureInitialized();
#if QT_VERSION >= 0x050000
    QDesignerFormWindowInterface * const m_form;
#else
    qdesigner_internal::FormWindowBase * const m_form;
#endif
    QStringList m_originalUiQrcPaths;
    ProjectExplorer::SessionNode *m_sessionNode;
    ProjectExplorer::NodesWatcher *m_sessionWatcher;
};

} // namespace Internal
} // namespace Designer

#endif // RESOURCEHANDLER_H
