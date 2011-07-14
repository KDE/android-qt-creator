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

#include "submiteditorfile.h"

using namespace VCSBase;
using namespace VCSBase::Internal;

/*!
    \class VCSBase::Internal::SubmitEditorFile

    \brief A non-saveable IFile for submit editor files.
*/

SubmitEditorFile::SubmitEditorFile(const QString &mimeType, QObject *parent) :
    Core::IFile(parent),
    m_mimeType(mimeType),
    m_modified(false)
{
}

void SubmitEditorFile::rename(const QString &newName)
{
    Q_UNUSED(newName);
    // We can't be renamed
    return;
}

void SubmitEditorFile::setFileName(const QString name)
{
     m_fileName = name;
     emit changed();
}

void SubmitEditorFile::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit changed();
}

bool SubmitEditorFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    emit saveMe(errorString, fileName, autoSave);
    if (!errorString->isEmpty())
        return false;
    emit changed();
    return true;
}

QString SubmitEditorFile::mimeType() const
{
    return m_mimeType;
}

Core::IFile::ReloadBehavior SubmitEditorFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool SubmitEditorFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}
