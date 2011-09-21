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

#ifndef QTWIZARD_H
#define QTWIZARD_H

#include "qtprojectparameters.h"
#include <projectexplorer/baseprojectwizarddialog.h>
#include <projectexplorer/customwizard/customwizard.h>

#include <coreplugin/basefilewizard.h>

#include <QtCore/QSet>

namespace Qt4ProjectManager {

class Qt4Project;
class TargetSetupPage;

namespace Internal {

class ModulesPage;

/* Base class for wizard creating Qt projects using QtProjectParameters.
 * To implement a project wizard, overwrite:
 * - createWizardDialog() to create up the dialog
 * - generateFiles() to set their contents
 * The base implementation provides the wizard parameters and opens
 * the finished project in postGenerateFiles().
 * The pro-file must be the last one of the generated files. */

class QtWizard : public Core::BaseFileWizard
{
    Q_OBJECT

protected:
    QtWizard(const QString &id,
             const QString &category,
             const QString &categoryTranslationScope,
             const QString &displayCategory,
             const QString &name,
             const QString &description,
             const QIcon &icon);

public:

    static QString templateDir();

    static QString sourceSuffix();
    static QString headerSuffix();
    static QString formSuffix();
    static QString profileSuffix();

    // Query CppTools settings for the class wizard settings
    static bool lowerCaseFiles();

    static bool qt4ProjectPostGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);

protected:
    static bool showModulesPageForApplications();
    static bool showModulesPageForLibraries();

private:
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);
};

// A custom wizard with an additional Qt 4 target page
class CustomQt4ProjectWizard : public ProjectExplorer::CustomProjectWizard {
    Q_OBJECT
public:
    explicit CustomQt4ProjectWizard(const Core::BaseFileWizardParameters& baseFileParameters,
                                    QObject *parent = 0);

    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;
    static void registerSelf();

protected:
    virtual bool postGenerateFiles(const QWizard *, const Core::GeneratedFiles &l, QString *errorMessage);

private:
    enum { targetPageId = 2 };
};

/* BaseQt4ProjectWizardDialog: Additionally offers modules page
 * and getter/setter for blank-delimited modules list, transparently
 * handling the visibility of the modules page list as well as a page
 * to select targets and Qt versions.
 */

class BaseQt4ProjectWizardDialog : public ProjectExplorer::BaseProjectWizardDialog {
    Q_OBJECT
protected:
    explicit BaseQt4ProjectWizardDialog(bool showModulesPage,
                                        Utils::ProjectIntroPage *introPage,
                                        int introId = -1,
                                        QWidget *parent = 0);
public:
    explicit BaseQt4ProjectWizardDialog(bool showModulesPage, QWidget *parent = 0);
    virtual ~BaseQt4ProjectWizardDialog();

    int addModulesPage(int id = -1);
    int addTargetSetupPage(QSet<QString> targets = QSet<QString>(), bool mobile = false, int id = -1);

    static QSet<QString> desktopTarget();

    QString selectedModules() const;
    void setSelectedModules(const QString &, bool lock = false);

    QString deselectedModules() const;
    void setDeselectedModules(const QString &);

    bool writeUserFile(const QString &proFileName) const;
    bool setupProject(Qt4Project *project) const;
    bool isTargetSelected(const QString &targetid) const;

private slots:
    void generateProfileName(const QString &name, const QString &path);

private:
    inline void init(bool showModulesPage);

    ModulesPage *m_modulesPage;
    TargetSetupPage *m_targetSetupPage;
    QString m_selectedModules;
    QString m_deselectedModules;
    QSet<QString> m_targets;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QTWIZARD_H
