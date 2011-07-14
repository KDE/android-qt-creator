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

#ifndef ADDLIBRARYWIZARD_H
#define ADDLIBRARYWIZARD_H

#include <utils/wizard.h>
#include <utils/pathchooser.h>

QT_BEGIN_NAMESPACE
class QRadioButton;
class QCheckBox;
class QLabel;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class LibraryDetailsWidget;
class LibraryDetailsController;
class LibraryTypePage;
class DetailsPage;
class SummaryPage;

namespace Ui {
    class LibraryDetailsWidget;
}

class AddLibraryWizard : public Utils::Wizard
{
    Q_OBJECT
public:
    enum LibraryKind {
        InternalLibrary,
        ExternalLibrary,
        SystemLibrary,
        PackageLibrary
        };

    enum LinkageType {
        DynamicLinkage,
        StaticLinkage,
        NoLinkage
        };

    enum MacLibraryType {
        FrameworkType,
        LibraryType,
        NoLibraryType
        };

    enum Platform {
        LinuxPlatform   = 0x01,
        MacPlatform     = 0x02,
        WindowsPlatform = 0x04,
        SymbianPlatform = 0x08
        };

    Q_DECLARE_FLAGS(Platforms, Platform)

    explicit AddLibraryWizard(const QString &fileName, QWidget *parent = 0);
    ~AddLibraryWizard();

    LibraryKind libraryKind() const;
    QString proFile() const;
    QString snippet() const;

signals:

private:
    LibraryTypePage *m_libraryTypePage;
    DetailsPage *m_detailsPage;
    SummaryPage *m_summaryPage;
    QString m_proFile;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AddLibraryWizard::Platforms)

class LibraryTypePage : public QWizardPage
{
    Q_OBJECT
public:
    LibraryTypePage(AddLibraryWizard *parent);
    AddLibraryWizard::LibraryKind libraryKind() const;

private:
    QRadioButton *m_internalRadio;
    QRadioButton *m_externalRadio;
    QRadioButton *m_systemRadio;
    QRadioButton *m_packageRadio;
};

class DetailsPage : public QWizardPage
{
    Q_OBJECT
public:
    DetailsPage(AddLibraryWizard *parent);
    virtual void initializePage();
    virtual bool isComplete() const;
    QString snippet() const;

private:
    AddLibraryWizard *m_libraryWizard;
    Ui::LibraryDetailsWidget *m_libraryDetailsWidget;
    LibraryDetailsController *m_libraryDetailsController;
};

class SummaryPage : public QWizardPage
{
    Q_OBJECT
public:
    SummaryPage(AddLibraryWizard *parent);
    virtual void initializePage();
    QString snippet() const;
private:
    AddLibraryWizard *m_libraryWizard;
    QLabel *m_summaryLabel;
    QLabel *m_snippetLabel;
    QString m_snippet;
};

class LibraryPathChooser : public Utils::PathChooser
{
    Q_OBJECT
public:
    LibraryPathChooser(QWidget *parent);
    virtual bool validatePath(const QString &path, QString *errorMessage);
};


} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ADDLIBRARYWIZARD_H
