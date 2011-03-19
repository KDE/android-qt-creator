/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "mobileappwizardpages.h"

#include "html5app.h"
#include "html5appwizard.h"
#include "html5appwizardpages.h"
#include "targetsetuppage.h"

#include "qt4projectmanagerconstants.h"

#include <QtCore/QCoreApplication>
#include <QtGui/QIcon>

namespace Qt4ProjectManager {
namespace Internal {

class Html5AppWizardDialog : public AbstractMobileAppWizardDialog
{
    Q_OBJECT

public:
    explicit Html5AppWizardDialog(QWidget *parent = 0);

private:
    class Html5AppWizardOptionsPage *m_htmlOptionsPage;
    friend class Html5AppWizard;
};

Html5AppWizardDialog::Html5AppWizardDialog(QWidget *parent)
    : AbstractMobileAppWizardDialog(parent)
    , m_htmlOptionsPage(0)
{
    setWindowTitle(tr("New HTML5 Application"));
    setIntroDescription(tr("This wizard generates a HTML5 application project."));

    m_htmlOptionsPage = new Html5AppWizardOptionsPage;
    addPageWithTitle(m_htmlOptionsPage, tr("HTML Options"));
}


class Html5AppWizardPrivate
{
    class Html5App *app;
    class Html5AppWizardDialog *wizardDialog;
    friend class Html5AppWizard;
};

Html5AppWizard::Html5AppWizard()
    : AbstractMobileAppWizard(parameters())
    , m_d(new Html5AppWizardPrivate)
{
    m_d->app = new Html5App;
    m_d->wizardDialog = 0;
}

Html5AppWizard::~Html5AppWizard()
{
    delete m_d->app;
    delete m_d;
}

Core::BaseFileWizardParameters Html5AppWizard::parameters()
{
    Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(QLatin1String(Constants::ICON_HTML5_APP)));
    parameters.setDisplayName(tr("HTML5 Application"));
    parameters.setId(QLatin1String("QA.HTML5A Application"));
    parameters.setDescription(tr("Creates an HTML5 application project that can contain "
                                 "both HTML5 and C++ code and includes a WebKit view.\n\n"
                                 "You can build the application and deploy it on desktop and "
                                 "mobile target platforms. For example, you can create signed "
                                 "Symbian Installation System (SIS) packages for this type of "
                                 "projects."));
    parameters.setCategory(QLatin1String(Constants::HTML5_WIZARD_CATEGORY));
    parameters.setDisplayCategory(QCoreApplication::translate(Constants::HTML5_WIZARD_TR_SCOPE,
                                                              Constants::HTML5_WIZARD_TR_CATEGORY));
    return parameters;
}

AbstractMobileAppWizardDialog *Html5AppWizard::createWizardDialogInternal(QWidget *parent) const
{
    m_d->wizardDialog = new Html5AppWizardDialog(parent);
    m_d->wizardDialog->m_htmlOptionsPage->setTouchOptimizationEndabled(
                m_d->app->touchOptimizedNavigationEnabled());
    return m_d->wizardDialog;
}

void Html5AppWizard::projectPathChanged(const QString &path) const
{
    m_d->wizardDialog->targetsPage()->setProFilePath(path);
}

void Html5AppWizard::prepareGenerateFiles(const QWizard *w,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)
    const Html5AppWizardDialog *wizard = qobject_cast<const Html5AppWizardDialog*>(w);
    m_d->app->setMainHtml(wizard->m_htmlOptionsPage->mainHtmlMode(),
                          wizard->m_htmlOptionsPage->mainHtmlData());
    m_d->app->setTouchOptimizedNavigationEnabled(
                wizard->m_htmlOptionsPage->touchOptimizationEndabled());
}

QString Html5AppWizard::fileToOpenPostGeneration() const
{
    return m_d->app->mainHtmlMode() == Html5App::ModeUrl ?
                m_d->app->path(AbstractMobileApp::MainCpp)
              : m_d->app->path(Html5App::MainHtml);
}

AbstractMobileApp *Html5AppWizard::app() const
{
    return m_d->app;
}

AbstractMobileAppWizardDialog *Html5AppWizard::wizardDialog() const
{
    return m_d->wizardDialog;
}

} // namespace Internal
} // namespace Qt4ProjectManager

#include "html5appwizard.moc"
