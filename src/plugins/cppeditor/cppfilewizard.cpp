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

#include "cppfilewizard.h"
#include "cppeditor.h"
#include "cppeditorconstants.h"

#include <cpptools/abstracteditorsupport.h>
#include <utils/codegeneration.h>

#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace CppEditor;
using namespace CppEditor::Internal;

enum { debugWizard = 0 };

CppFileWizard::CppFileWizard(const BaseFileWizardParameters &parameters,
                             FileType type,
                             QObject *parent) :
    Core::StandardFileWizard(parameters, parent),
    m_type(type)
{
}

Core::GeneratedFiles CppFileWizard::generateFilesFromPath(const QString &path,
                                                          const QString &name,
                                                          QString * /*errorMessage*/) const

{
    const QString mimeType = m_type == Source ? QLatin1String(Constants::CPP_SOURCE_MIMETYPE) : QLatin1String(Constants::CPP_HEADER_MIMETYPE);
    const QString fileName = Core::BaseFileWizard::buildFileName(path, name, preferredSuffix(mimeType));

    Core::GeneratedFile file(fileName);
    file.setContents(fileContents(m_type, fileName));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << file;
}

QString CppFileWizard::fileContents(FileType type, const QString &fileName) const
{
    QString contents;
    QTextStream str(&contents);
    str << CppTools::AbstractEditorSupport::licenseTemplate(fileName);
    switch (type) {
    case Header: {
            const QString guard = Utils::headerGuard(fileName);
            str << QLatin1String("#ifndef ") << guard
                << QLatin1String("\n#define ") <<  guard <<  QLatin1String("\n\n#endif // ")
                << guard << QLatin1String("\n");
        }
        break;
    case Source:
        str << '\n';
        break;
    }
    return contents;
}
