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

#include "glsleditorfactory.h"
#include "glsleditoreditable.h"
#include "glsleditor.h"
#include "glsleditoractionhandler.h"
#include "glsleditorconstants.h"
#include "glsleditorplugin.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QMainWindow>

using namespace GLSLEditor::Internal;
using namespace GLSLEditor::Constants;

GLSLEditorFactory::GLSLEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent)
{
    m_mimeTypes
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE)
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_VERT)
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG)
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_VERT_ES)
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG_ES)
            ;
}

Core::Id GLSLEditorFactory::id() const
{
    return C_GLSLEDITOR_ID;
}

QString GLSLEditorFactory::displayName() const
{
    return tr(C_GLSLEDITOR_DISPLAY_NAME);
}

Core::IFile *GLSLEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    if (!iface) {
        qWarning() << "QmlEditorFactory::open: openEditor failed for " << fileName;
        return 0;
    }
    return iface->file();
}

Core::IEditor *GLSLEditorFactory::createEditor(QWidget *parent)
{
    GLSLEditor::GLSLTextEditorWidget *rc = new GLSLEditor::GLSLTextEditorWidget(parent);
    GLSLEditorPlugin::instance()->initializeEditor(rc);
    return rc->editor();
}

QStringList GLSLEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

void GLSLEditorFactory::updateEditorInfoBar(Core::IEditor *)
{
}
