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

#include "qmlprojectfile.h"
#include "qmlproject.h"
#include "qmlprojectconstants.h"
#include <utils/qtcassert.h>

namespace QmlProjectManager {
namespace Internal {

QmlProjectFile::QmlProjectFile(QmlProject *parent, QString fileName)
    : Core::IFile(parent),
      m_project(parent),
      m_fileName(fileName)
{
    QTC_CHECK(m_project);
    QTC_CHECK(!fileName.isEmpty());
}

QmlProjectFile::~QmlProjectFile()
{
}

bool QmlProjectFile::save(QString *, const QString &, bool)
{
    return false;
}

void QmlProjectFile::rename(const QString &newName)
{
    // Can't happen...
    Q_UNUSED(newName);
    Q_ASSERT(false);
}

QString QmlProjectFile::fileName() const
{
    return m_fileName;
}

QString QmlProjectFile::defaultPath() const
{
    return QString();
}

QString QmlProjectFile::suggestedFileName() const
{
    return QString();
}

QString QmlProjectFile::mimeType() const
{
    return Constants::QMLMIMETYPE;
}

bool QmlProjectFile::isModified() const
{
    return false;
}

bool QmlProjectFile::isReadOnly() const
{
    return true;
}

bool QmlProjectFile::isSaveAsAllowed() const
{
    return false;
}

Core::IFile::ReloadBehavior QmlProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool QmlProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)

    if (type == TypeContents)
        m_project->refreshProjectFile();

    return true;
}

} // namespace Internal
} // namespace QmlProjectManager
