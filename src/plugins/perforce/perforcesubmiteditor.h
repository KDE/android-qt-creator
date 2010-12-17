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

#ifndef PERFORCESUBMITEDITOR_H
#define PERFORCESUBMITEDITOR_H

#include <vcsbase/vcsbasesubmiteditor.h>

#include <QtCore/QStringList>
#include <QtCore/QMap>

namespace VCSBase {
    class SubmitFileModel;
}

namespace Perforce {
namespace Internal {

class PerforceSubmitEditorWidget;
class PerforcePlugin;

/* PerforceSubmitEditor: In p4, the file list is contained in the
 * submit message file (change list). On setting the file contents,
 * it is split apart in message and file list and re-assembled
 * when retrieving the file list.
 * As a p4 submit starts with all opened files, there is API to restrict
 * the file list to current project files in question
 * (restrictToProjectFiles()). */
class PerforceSubmitEditor : public VCSBase::VCSBaseSubmitEditor
{
    Q_OBJECT

public:
    explicit PerforceSubmitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters, QWidget *parent);

    /* The p4 submit starts with all opened files. Restrict
     * it to the current project files in question. */
    void restrictToProjectFiles(const QStringList &files);

    static QString fileFromChangeLine(const QString &line);

protected:
    virtual QString fileContents() const;
    virtual bool setFileContents(const QString &contents);

private:
    inline PerforceSubmitEditorWidget *submitEditorWidget();
    bool parseText(QString text);
    void updateFields();
    void updateEntries();

    QMap<QString, QString> m_entries;
    VCSBase::SubmitFileModel *m_fileModel;
};

} // namespace Internal
} // namespace Perforce

#endif // PERFORCESUBMITEDITOR_H
