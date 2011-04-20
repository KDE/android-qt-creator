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

#include "resourcewizard.h"
#include "resourceeditorw.h"
#include "resourceeditorconstants.h"

using namespace ResourceEditor;
using namespace ResourceEditor::Internal;

ResourceWizard::ResourceWizard(const BaseFileWizardParameters &parameters, QObject *parent)
  : Core::StandardFileWizard(parameters, parent)
{
}

Core::GeneratedFiles
ResourceWizard::generateFilesFromPath(const QString &path,
                                      const QString &name,
                                      QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const QString suffix = preferredSuffix(QLatin1String(Constants::C_RESOURCE_MIMETYPE));
    const QString fileName = Core::BaseFileWizard::buildFileName(path, name, suffix);
    Core::GeneratedFile file(fileName);
    file.setContents(QLatin1String("<RCC/>"));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << file;
}
