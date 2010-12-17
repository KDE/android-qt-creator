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

#include "vcsannotatetaskhandler.h"

#include "task.h"
#include "projectexplorerconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <QtCore/QFileInfo>

#include <QtGui/QAction>

using namespace ProjectExplorer::Internal;

VcsAnnotateTaskHandler::VcsAnnotateTaskHandler() :
    ITaskHandler(QLatin1String(Constants::VCS_ANNOTATE_TASK))
{ }

bool VcsAnnotateTaskHandler::canHandle(const ProjectExplorer::Task &task)
{
    QFileInfo fi(task.file);
    if (!fi.exists() || !fi.isFile() || !fi.isReadable())
        return false;
    Core::IVersionControl *vc = Core::ICore::instance()->vcsManager()->findVersionControlForDirectory(fi.absolutePath());
    if (!vc)
        return false;
    return vc->supportsOperation(Core::IVersionControl::AnnotateOperation);
}

void VcsAnnotateTaskHandler::handle(const ProjectExplorer::Task &task)
{
    QFileInfo fi(task.file);
    Core::IVersionControl *vc = Core::ICore::instance()->vcsManager()->findVersionControlForDirectory(fi.absolutePath());
    Q_ASSERT(vc);
    Q_ASSERT(vc->supportsOperation(Core::IVersionControl::AnnotateOperation));
    vc->vcsAnnotate(fi.absoluteFilePath(), task.line);
}

QAction *VcsAnnotateTaskHandler::createAction(QObject *parent)
{
    QAction *vcsannotateAction = new QAction(tr("&Annotate"), parent);
    vcsannotateAction->setToolTip("Annotate using version control system");
    return vcsannotateAction;
}
