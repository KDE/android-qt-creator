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

#ifndef LIBRARYWIZARDDIALOG_H
#define LIBRARYWIZARDDIALOG_H

#include "qtwizard.h"
#include "qtprojectparameters.h"

namespace Qt4ProjectManager {
namespace Internal {

struct QtProjectParameters;
class FilesPage;
class MobileLibraryWizardOptionPage;
struct LibraryParameters;
struct MobileLibraryParameters;

// Library wizard dialog.
class LibraryWizardDialog : public BaseQt4ProjectWizardDialog
{
    Q_OBJECT

public:
    LibraryWizardDialog(const QString &templateName,
                        const QIcon &icon,
                        const QList<QWizardPage*> &extensionPages,
                        bool showModulesPage,
                        QWidget *parent = 0);

    void setSuffixes(const QString &header, const QString &source,  const QString &form= QString());
    void setLowerCaseFiles(bool);
    void setSymbianUid(const QString &uid);

    QtProjectParameters parameters() const;
    LibraryParameters libraryParameters() const;
    MobileLibraryParameters mobileLibraryParameters() const;

    virtual int nextId() const;

protected:
    void initializePage(int id);
    void cleanupPage(int id);

private slots:
    void slotCurrentIdChanged(int);

private:
    QtProjectParameters::Type type() const;
    void setupFilesPage();
    void setupMobilePage();
    bool isModulesPageSkipped() const;
    int skipModulesPageIfNeeded() const;

    FilesPage *m_filesPage;
    MobileLibraryWizardOptionPage *m_mobilePage;
    bool m_pluginBaseClassesInitialized;
    int m_filesPageId;
    int m_modulesPageId;
    int m_targetPageId;
    int m_mobilePageId;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // LIBRARYWIZARDDIALOG_H
