/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "formeditorplugin.h"
#include "formeditorfactory.h"
#include "formeditorw.h"
#include "formwizard.h"

#ifdef CPP_ENABLED
#  include "formclasswizard.h"
#  include "cppsettingspage.h"
#endif

#include "settingspage.h"
#include "designerconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>
#include <QtCore/QtPlugin>

#ifdef CPP_ENABLED
#    include <QtGui/QWizard>
#    include <QtGui/QMainWindow>
#endif

using namespace Designer::Internal;
using namespace Designer::Constants;

FormEditorPlugin::FormEditorPlugin()
{
}

FormEditorPlugin::~FormEditorPlugin()
{
    FormEditorW::deleteInstance();
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
bool FormEditorPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/formeditor/Designer.mimetypes.xml"), error))
        return false;

    initializeTemplates();

    addAutoReleasedObject(new FormEditorFactory);
    addAutoReleasedObject(new SettingsPageProvider);

    // Ensure that loading designer translations is done before FormEditorW is instantiated
    const QString locale = Core::ICore::instance()->userInterfaceLanguage();
    if (!locale.isEmpty()) {
        QTranslator *qtr = new QTranslator(this);
        const QString &creatorTrPath =
                Core::ICore::instance()->resourcePath() + QLatin1String("/translations");
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &trFile = QLatin1String("designer_") + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            qApp->installTranslator(qtr);
    }
    error->clear();
    return true;
}

void FormEditorPlugin::extensionsInitialized()
{
    // 4) test and make sure everything works (undo, saving, editors, opening/closing multiple files, dirtiness etc)
}

////////////////////////////////////////////////////
//
// PRIVATE methods
//
////////////////////////////////////////////////////

void FormEditorPlugin::initializeTemplates()
{
    FormWizard::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizardParameters.setDisplayCategory(QCoreApplication::translate("Core", Core::Constants::WIZARD_TR_CATEGORY_QT));
    const QString formFileType = QLatin1String(Constants::FORM_FILE_TYPE);
    wizardParameters.setDisplayName(tr("Qt Designer Form"));
    wizardParameters.setId(QLatin1String("D.Form"));
    wizardParameters.setDescription(tr("Creates a Qt Designer form that you can add to a Qt C++ project. "
                                       "This is useful if you already have an existing class for the UI business logic."));
    addAutoReleasedObject(new FormWizard(wizardParameters, this));

#ifdef CPP_ENABLED
    wizardParameters.setKind(Core::IWizard::ClassWizard);
    wizardParameters.setDisplayName(tr("Qt Designer Form Class"));
    wizardParameters.setId(QLatin1String("C.FormClass"));
    wizardParameters.setDescription(tr("Creates a Qt Designer form along with a matching class (C++ header and source file) "
                                       "for implementation purposes. You can add the form and class to an existing Qt C++ Project."));
    addAutoReleasedObject(new FormClassWizard(wizardParameters, this));
    addAutoReleasedObject(new CppSettingsPage);
#endif
}

Q_EXPORT_PLUGIN(FormEditorPlugin)
