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

#include "basevcssubmiteditorfactory.h"
#include "vcsbasesubmiteditor.h"

#include <coreplugin/editormanager/editormanager.h>

namespace VCSBase {

struct BaseVCSSubmitEditorFactoryPrivate
{
    BaseVCSSubmitEditorFactoryPrivate(const VCSBaseSubmitEditorParameters *parameters);

    const VCSBaseSubmitEditorParameters *m_parameters;
    const QString m_id;
    const QString m_displayName;
    const QStringList m_mimeTypes;
};

BaseVCSSubmitEditorFactoryPrivate::BaseVCSSubmitEditorFactoryPrivate(const VCSBaseSubmitEditorParameters *parameters) :
    m_parameters(parameters),
    m_id(parameters->id),
    m_displayName(parameters->displayName),
    m_mimeTypes(QLatin1String(parameters->mimeType))
{
}

BaseVCSSubmitEditorFactory::BaseVCSSubmitEditorFactory(const VCSBaseSubmitEditorParameters *parameters) :
    m_d(new BaseVCSSubmitEditorFactoryPrivate(parameters))
{
}

BaseVCSSubmitEditorFactory::~BaseVCSSubmitEditorFactory()
{
    delete m_d;
}

Core::IEditor *BaseVCSSubmitEditorFactory::createEditor(QWidget *parent)
{
    return createBaseSubmitEditor(m_d->m_parameters, parent);
}

QString BaseVCSSubmitEditorFactory::id() const
{
    return m_d->m_id;
}

QString BaseVCSSubmitEditorFactory::displayName() const
{
    return m_d->m_displayName;
}


QStringList BaseVCSSubmitEditorFactory::mimeTypes() const
{
    return m_d->m_mimeTypes;
}

Core::IFile *BaseVCSSubmitEditorFactory::open(const QString &fileName)
{
    if (Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id()))
        return iface->file();
    return 0;
}

} // namespace VCSBase
