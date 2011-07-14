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

#ifndef SUBMITEDITORFILE_H
#define SUBMITEDITORFILE_H

#include <coreplugin/ifile.h>

namespace VCSBase {
namespace Internal {

class SubmitEditorFile : public Core::IFile
{
    Q_OBJECT
public:
    explicit SubmitEditorFile(const QString &mimeType,
                              QObject *parent = 0);

    QString fileName() const { return m_fileName; }
    QString defaultPath() const { return QString(); }
    QString suggestedFileName() const { return QString(); }

    bool isModified() const { return m_modified; }
    virtual QString mimeType() const;
    bool isReadOnly() const { return false; }
    bool isSaveAsAllowed() const { return false; }
    bool save(QString *errorString, const QString &fileName, bool autoSave);
    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    void rename(const QString &newName);

    void setFileName(const QString name);
    void setModified(bool modified = true);

signals:
    void saveMe(QString *errorString, const QString &fileName, bool autoSave);

private:
    const QString m_mimeType;
    bool m_modified;
    QString m_fileName;
};


} // namespace Internal
} // namespace VCSBase

#endif // SUBMITEDITORFILE_H
