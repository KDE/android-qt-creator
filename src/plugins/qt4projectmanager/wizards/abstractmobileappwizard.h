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

#ifndef ABSTRACTMOBILEAPPWIZARD_H
#define ABSTRACTMOBILEAPPWIZARD_H

#include <qt4projectmanager/qt4projectmanager_global.h>
#include <qtsupport/qtversionmanager.h>
#include <coreplugin/basefilewizard.h>
#include <projectexplorer/baseprojectwizarddialog.h>
#include <qt4projectmanager/wizards/abstractmobileapp.h>

namespace Qt4ProjectManager {

class AbstractMobileApp;
class TargetSetupPage;

namespace Internal {
class MobileAppWizardGenericOptionsPage;
class MobileAppWizardSymbianOptionsPage;
class MobileAppWizardMaemoOptionsPage;
class MobileAppWizardHarmattanOptionsPage;
}

/// \internal
class QT4PROJECTMANAGER_EXPORT AbstractMobileAppWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT

protected:
    explicit AbstractMobileAppWizardDialog(QWidget *parent, const QtSupport::QtVersionNumber &minimumQtVersionNumber);
    void addMobilePages();

public:
    TargetSetupPage *targetsPage() const;

protected:
    int addPageWithTitle(QWizardPage *page, const QString &title);
    virtual void initializePage(int id);
    virtual void cleanupPage(int id);
    virtual void setIgnoreGenericOptionsPage(bool);
    virtual int nextId() const;

    Utils::WizardProgressItem *targetsPageItem() const;

private:
    int idOfNextGenericPage() const;
    Utils::WizardProgressItem *itemOfNextGenericPage() const;
    bool isSymbianTargetSelected() const;
    bool isFremantleTargetSelected() const;
    bool isHarmattanTargetSelected() const;
    bool isMeegoTargetSelected() const;

    Internal::MobileAppWizardGenericOptionsPage *m_genericOptionsPage;
    Internal::MobileAppWizardSymbianOptionsPage *m_symbianOptionsPage;
    Internal::MobileAppWizardMaemoOptionsPage *m_maemoOptionsPage;
    Internal::MobileAppWizardHarmattanOptionsPage *m_harmattanOptionsPage;
    TargetSetupPage *m_targetsPage;

    int m_genericOptionsPageId;
    int m_symbianOptionsPageId;
    int m_maemoOptionsPageId;
    int m_harmattanOptionsPageId;
    int m_targetsPageId;
    bool m_ignoreGeneralOptions; // If true, do not show generic mobile options page.
    Utils::WizardProgressItem *m_targetItem;
    Utils::WizardProgressItem *m_genericItem;
    Utils::WizardProgressItem *m_symbianItem;
    Utils::WizardProgressItem *m_maemoItem;
    Utils::WizardProgressItem *m_harmattanItem;

    friend class AbstractMobileAppWizard;
};

/// \internal
class QT4PROJECTMANAGER_EXPORT AbstractMobileAppWizard : public Core::BaseFileWizard
{
    Q_OBJECT
protected:
    explicit AbstractMobileAppWizard(const Core::BaseFileWizardParameters &params,
        QObject *parent = 0);

private slots:
    void useProjectPath(const QString &projectName, const QString &projectPath);

protected:
    virtual QString fileToOpenPostGeneration() const = 0;

private:
    virtual QWizard *createWizardDialog(QWidget *parent,
        const QString &defaultPath, const WizardPageList &extensionPages) const;
    virtual Core::GeneratedFiles generateFiles(const QWizard *wizard,
        QString *errorMessage) const;
    virtual bool postGenerateFiles(const QWizard *w,
        const Core::GeneratedFiles &l, QString *errorMessage);

    virtual AbstractMobileApp *app() const=0;
    virtual AbstractMobileAppWizardDialog *wizardDialog() const=0;
    virtual AbstractMobileAppWizardDialog *createWizardDialogInternal(QWidget *parent) const=0;
    virtual void projectPathChanged(const QString &path) const=0;
    virtual void prepareGenerateFiles(const QWizard *wizard,
        QString *errorMessage) const=0;
};

} // namespace Qt4ProjectManager

#endif // ABSTRACTMOBILEAPPWIZARD_H
