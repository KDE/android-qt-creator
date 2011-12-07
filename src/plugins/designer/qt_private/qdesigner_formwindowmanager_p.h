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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QDESIGNER_FORMWINDOMANAGER_H
#define QDESIGNER_FORMWINDOMANAGER_H

#include "shared_global_p.h"
#include <QtDesigner/QDesignerFormWindowManagerInterface>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class PreviewManager;

//
// Convenience methods to manage form previews (ultimately forwarded to PreviewManager).
//
class QDESIGNER_SHARED_EXPORT QDesignerFormWindowManager
    : public QDesignerFormWindowManagerInterface
{
    Q_OBJECT
public:
    explicit QDesignerFormWindowManager(QObject *parent = 0);
    virtual ~QDesignerFormWindowManager();

    virtual QAction *actionDefaultPreview() const;
    virtual QActionGroup *actionGroupPreviewInStyle() const;
    virtual QAction *actionShowFormWindowSettingsDialog() const;

    virtual QPixmap createPreviewPixmap(QString *errorMessage) = 0;

    virtual PreviewManager *previewManager() const = 0;

Q_SIGNALS:
    void formWindowSettingsChanged(QDesignerFormWindowInterface *fw);

public Q_SLOTS:
    virtual void closeAllPreviews() = 0;
    void aboutPlugins();

private:
    void *m_unused;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // QDESIGNER_FORMWINDOMANAGER_H
