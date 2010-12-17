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

#include "formeditorfactory.h"
#include "formeditorw.h"
#include "formwindoweditor.h"
#include "editordata.h"
#include "designerconstants.h"
#include "designerxmleditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/modemanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace Designer::Constants;

namespace Designer {
namespace Internal {

FormEditorFactory::FormEditorFactory()
  : Core::IEditorFactory(Core::ICore::instance()),
    m_mimeTypes(QLatin1String(FORM_MIMETYPE))
{
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(":/formeditor/images/qt_ui.png")),
                                               QLatin1String("ui"));
    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
             SLOT(updateEditorInfoBar(Core::IEditor*)));
}

QString FormEditorFactory::id() const
{
    return QLatin1String(DESIGNER_XML_EDITOR_ID);
}

QString FormEditorFactory::displayName() const
{
    return tr(C_DESIGNER_XML_DISPLAY_NAME);
}

Core::IFile *FormEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    return iface ? iface->file() : 0;
}

Core::IEditor *FormEditorFactory::createEditor(QWidget *parent)
{
    const EditorData data = FormEditorW::instance()->createEditor(parent);
    return data.formWindowEditor;
}

QStringList FormEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

void FormEditorFactory::updateEditorInfoBar(Core::IEditor *editor)
{
    if (qobject_cast<FormWindowEditor *>(editor)) {
        Core::EditorManager::instance()->showEditorInfoBar(Constants::INFO_READ_ONLY,
            tr("This file can only be edited in <b>Design</b> mode."),
            tr("Switch mode"), this, SLOT(designerModeClicked()));
    } else {
        Core::EditorManager::instance()->hideEditorInfoBar(Constants::INFO_READ_ONLY);
    }
}

void FormEditorFactory::designerModeClicked()
{
    Core::ICore::instance()->modeManager()->activateMode(QLatin1String(Core::Constants::MODE_DESIGN));
}

} // namespace Internal
} // namespace Designer


