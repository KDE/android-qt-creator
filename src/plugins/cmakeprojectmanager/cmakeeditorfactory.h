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

#ifndef CMAKEEDITORFACTORY_H
#define CMAKEEDITORFACTORY_H

#include <coreplugin/editormanager/ieditorfactory.h>
#include "cmakeprojectmanager.h"

#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace TextEditor {
class TextEditorActionHandler;
}

namespace CMakeProjectManager {

namespace Internal {

class CMakeEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    CMakeEditorFactory(CMakeManager *parent, TextEditor::TextEditorActionHandler *handler);
    ~CMakeEditorFactory();

    virtual QStringList mimeTypes() const;
    virtual QString id() const;
    virtual QString displayName() const;
    Core::IFile *open(const QString &fileName);
    Core::IEditor *createEditor(QWidget *parent);

private:
    const QStringList m_mimeTypes;
    CMakeManager *m_manager;
    TextEditor::TextEditorActionHandler *m_actionHandler;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEEDITORFACTORY_H
