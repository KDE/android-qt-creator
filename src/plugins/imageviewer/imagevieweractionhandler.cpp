/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2010 Denis Mingulov.
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

#include "imagevieweractionhandler.h"
#include "imageviewer.h"
#include "imageviewerconstants.h"

#include <QtCore/QList>
#include <QtCore/QSignalMapper>
#include <QtGui/QAction>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/id.h>

namespace ImageViewer {
namespace Internal {

enum SupportedActions { ZoomIn = 0, ZoomOut, OriginalSize, FitToScreen, Background, Outline };

ImageViewerActionHandler::ImageViewerActionHandler(QObject *parent) :
    QObject(parent), m_signalMapper(new QSignalMapper(this))
{
    connect(m_signalMapper, SIGNAL(mapped(int)), SLOT(actionTriggered(int)));
}

void ImageViewerActionHandler::actionTriggered(int supportedAction)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    Core::IEditor *editor = editorManager->currentEditor();
    ImageViewer *viewer = qobject_cast<ImageViewer *>(editor);
    if (!viewer)
        return;

    SupportedActions action = static_cast<SupportedActions>(supportedAction);
    switch(action) {
    case ZoomIn:
        viewer->zoomIn();
        break;
    case ZoomOut:
        viewer->zoomOut();
        break;
    case OriginalSize:
        viewer->resetToOriginalSize();
        break;
    case FitToScreen:
        viewer->fitToScreen();
        break;
    case Background:
        viewer->switchViewBackground();
        break;
    case Outline:
        viewer->switchViewOutline();
        break;
    default:
        break;
    }
}

void ImageViewerActionHandler::createActions()
{
    registerNewAction(ZoomIn, Constants::ACTION_ZOOM_IN, tr("Zoom In"),
                      QKeySequence(tr("Ctrl++")));
    registerNewAction(ZoomOut, Constants::ACTION_ZOOM_OUT, tr("Zoom Out"),
                      QKeySequence(tr("Ctrl+-")));
    registerNewAction(OriginalSize, Constants::ACTION_ORIGINAL_SIZE, tr("Original Size"),
                      QKeySequence(tr("Ctrl+0")));
    registerNewAction(FitToScreen, Constants::ACTION_FIT_TO_SCREEN, tr("Fit To Screen"),
                      QKeySequence(tr("Ctrl+=")));
    registerNewAction(Background, Constants::ACTION_BACKGROUND, tr("Switch Background"),
                      QKeySequence(tr("Ctrl+[")));
    registerNewAction(Outline, Constants::ACTION_OUTLINE, tr("Switch Outline"),
                      QKeySequence(tr("Ctrl+]")));
}

void ImageViewerActionHandler::registerNewAction(int actionId, const Core::Id &id,
    const QString &title, const QKeySequence &key)
{
    Core::Context context(Constants::IMAGEVIEWER_ID);
    Core::ActionManager *actionManager = Core::ICore::instance()->actionManager();
    QAction *action = new QAction(title, this);
    Core::Command *command = actionManager->registerAction(action, id, context);
    if (command)
        command->setDefaultKeySequence(key);
    connect(action, SIGNAL(triggered()), m_signalMapper, SLOT(map()));
    m_signalMapper->setMapping(action, actionId);
}

} // namespace Internal
} // namespace ImageViewer
