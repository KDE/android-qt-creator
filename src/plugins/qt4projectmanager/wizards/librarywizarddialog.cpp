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

#include "librarywizarddialog.h"
#include "filespage.h"
#include "libraryparameters.h"
#include "modulespage.h"
#include "mobilelibrarywizardoptionpage.h"
#include "mobilelibraryparameters.h"
#include "abstractmobileapp.h"
#include "qt4projectmanagerconstants.h"

#include <utils/projectintropage.h>

#include <QtCore/QDebug>

#include <QtGui/QComboBox>
#include <QtGui/QLabel>

enum { debugLibWizard = 0 };

namespace Qt4ProjectManager {
namespace Internal {

struct PluginBaseClasses {
    const char *name;
    const char *module;
    // blank separated list or 0
    const char *dependentModules;
    const char *targetDirectory;
};

static const PluginBaseClasses pluginBaseClasses[] =
{
    { "QAccessiblePlugin", "QtGui", "QtCore", "accessible" },
    { "QDecorationPlugin", "QtGui", "QtCore", 0},
    { "QIconEnginePluginV2", "QtGui", "QtCore", "imageformats" },
    { "QImageIOPlugin", "QtGui", "QtCore", "imageformats" },
    { "QScriptExtensionPlugin", "QtScript", "QtCore", 0 },
    { "QSqlDriverPlugin", "QtSql", "QtCore", "sqldrivers" },
    { "QStylePlugin", "QtGui", "QtCore", "styles" },
    { "QTextCodecPlugin", "QtCore", 0, "codecs" }
};

enum { defaultPluginBaseClass = 6 };

static const PluginBaseClasses *findPluginBaseClass(const QString &name)
{
    const int pluginBaseClassCount = sizeof(pluginBaseClasses)/sizeof(PluginBaseClasses);
    for (int i = 0; i < pluginBaseClassCount; i++)
        if (name == QLatin1String(pluginBaseClasses[i].name))
            return pluginBaseClasses + i;
    return 0;
}

// return dependencies of a plugin as a line ready for the 'QT=' line in a pro
// file
static QString pluginDependencies(const PluginBaseClasses *plb)
{
    QString dependencies;
    const QChar blank = QLatin1Char(' ');
    // Find the module names and convert to ids
    QStringList pluginModules= plb->dependentModules ?
                               QString(QLatin1String(plb->dependentModules)).split(blank) :
                               QStringList();
    pluginModules.push_back(QLatin1String(plb->module));
    foreach (const QString &module, pluginModules) {
        if (!dependencies.isEmpty())
            dependencies += blank;
        dependencies += ModulesPage::idOfModule(module);
    }
    return dependencies;
}

// A Project intro page with an additional type chooser.
class LibraryIntroPage : public Utils::ProjectIntroPage
{
    Q_DISABLE_COPY(LibraryIntroPage)
public:
    explicit LibraryIntroPage(QWidget *parent = 0);

    QtProjectParameters::Type type() const;

private:
    QComboBox *m_typeCombo;
};

LibraryIntroPage::LibraryIntroPage(QWidget *parent) :
    Utils::ProjectIntroPage(parent),
    m_typeCombo(new QComboBox)
{
    m_typeCombo->setEditable(false);
    m_typeCombo->addItem(LibraryWizardDialog::tr("Shared Library"),
                         QVariant(QtProjectParameters::SharedLibrary));
    m_typeCombo->addItem(LibraryWizardDialog::tr("Statically Linked Library"),
                         QVariant(QtProjectParameters::StaticLibrary));
    m_typeCombo->addItem(LibraryWizardDialog::tr("Qt 4 Plugin"),
                         QVariant(QtProjectParameters::Qt4Plugin));
    insertControl(0, new QLabel(LibraryWizardDialog::tr("Type")), m_typeCombo);
}

QtProjectParameters::Type LibraryIntroPage::type() const
{
    return static_cast<QtProjectParameters::Type>(m_typeCombo->itemData(m_typeCombo->currentIndex()).toInt());
}

// ------------------- LibraryWizardDialog
LibraryWizardDialog::LibraryWizardDialog(const QString &templateName,
                                         const QIcon &icon,
                                         const QList<QWizardPage*> &extensionPages,
                                         bool showModulesPage,
                                         QWidget *parent) :
    BaseQt4ProjectWizardDialog(showModulesPage, new LibraryIntroPage, -1, parent),
    m_filesPage(new FilesPage),
    m_mobilePage(new MobileLibraryWizardOptionPage),
    m_pluginBaseClassesInitialized(false),
    m_filesPageId(-1), m_modulesPageId(-1), m_targetPageId(-1),
    m_mobilePageId(-1)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    setSelectedModules(QLatin1String("core"));

    // Note that QWizard::currentIdChanged() is emitted at strange times.
    // Use the intro page instead, set up initially
    setIntroDescription(tr("This wizard generates a C++ library project."));

    m_targetPageId = addTargetSetupPage();
    Utils::WizardProgressItem *targetItem = wizardProgress()->item(m_targetPageId);

    m_mobilePageId = addPage(m_mobilePage);
    Utils::WizardProgressItem *mobileItem = wizardProgress()->item(m_mobilePageId);
    mobileItem->setTitle(QLatin1String("    ") + tr("Symbian Specific"));

    m_modulesPageId = addModulesPage();
    Utils::WizardProgressItem *modulesItem = wizardProgress()->item(m_modulesPageId);

    targetItem->setNextItems(QList<Utils::WizardProgressItem *>()
                             << mobileItem << modulesItem);
    targetItem->setNextShownItem(0);

    m_filesPage->setNamespacesEnabled(true);
    m_filesPage->setFormFileInputVisible(false);
    m_filesPage->setClassTypeComboVisible(false);

    m_filesPageId = addPage(m_filesPage);
    wizardProgress()->item(m_filesPageId)->setTitle(tr("Details"));

    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotCurrentIdChanged(int)));

    foreach (QWizardPage *p, extensionPages)
        Core::BaseFileWizard::applyExtensionPageShortTitle(this, addPage(p));
}

void LibraryWizardDialog::setSuffixes(const QString &header, const QString &source,  const QString &form)
{
    m_filesPage->setSuffixes(header, source, form);
}

void LibraryWizardDialog::setLowerCaseFiles(bool l)
{
    m_filesPage->setLowerCaseFiles(l);
}

void LibraryWizardDialog::setSymbianUid(const QString &uid)
{
    m_mobilePage->setSymbianUid(uid);
}

QtProjectParameters::Type  LibraryWizardDialog::type() const
{
    return static_cast<const LibraryIntroPage*>(introPage())->type();
}

int LibraryWizardDialog::nextId() const
{
    //if there was no Symbian target defined we omit "Symbian specific" step
    //we omit this step if the library type is not dll
    if (currentId() == m_filesPageId) {
        bool symbianTargetEnabled = isTargetSelected(QLatin1String(Constants::S60_DEVICE_TARGET_ID))
                || isTargetSelected(QLatin1String(Constants::S60_EMULATOR_TARGET_ID));
        if (!symbianTargetEnabled || type() != QtProjectParameters::SharedLibrary) {
            QList<int> ids = pageIds();
            int mobileIndex = ids.lastIndexOf(m_mobilePageId);
            if (mobileIndex>=0)
                return ids[mobileIndex+1];
        }
    }
    // When leaving the intro or target page, the modules page is skipped
    // in the case of a plugin since it knows its dependencies by itself.
    const int m_beforeModulesPageId = m_targetPageId != -1 ? m_targetPageId : 0;
    if (currentId() != m_beforeModulesPageId)
        return BaseQt4ProjectWizardDialog::nextId();
    if (type() != QtProjectParameters::Qt4Plugin && m_modulesPageId != -1)
        return m_modulesPageId;
    return m_filesPageId;
}

QtProjectParameters LibraryWizardDialog::parameters() const
{
    QtProjectParameters rc;
    rc.type = type();
    rc.fileName = projectName();
    rc.path = path();
    if (rc.type == QtProjectParameters::Qt4Plugin) {
        // Plugin: Dependencies & Target directory
        if (const PluginBaseClasses *plb = findPluginBaseClass(m_filesPage->baseClassName())) {
            rc.selectedModules = pluginDependencies(plb);
            if (plb->targetDirectory) {
                rc.targetDirectory = QLatin1String("$$[QT_INSTALL_PLUGINS]/");
                rc.targetDirectory += QLatin1String(plb->targetDirectory);
            }
        }
    } else {
        // Modules from modules page
        rc.selectedModules = selectedModules();
        rc.deselectedModules = deselectedModules();
    }
    return rc;
}

void LibraryWizardDialog::slotCurrentIdChanged(int id)
{
    if (debugLibWizard)
        qDebug() << Q_FUNC_INFO << id;
    if (id == m_filesPageId)
        setupFilesPage();// Switching to files page: Set up base class accordingly (plugin)
    else if (id == m_mobilePageId
             || (m_mobilePage->symbianUid().isEmpty()
             && currentPage() && currentPage()->isFinalPage()))
        setupMobilePage();
}

void LibraryWizardDialog::setupFilesPage()
{
    switch (type()) {
    case QtProjectParameters::Qt4Plugin:
        if (!m_pluginBaseClassesInitialized) {
            if (debugLibWizard)
                qDebug("initializing for plugins");
            QStringList baseClasses;
            const int pluginBaseClassCount = sizeof(pluginBaseClasses)/sizeof(PluginBaseClasses);
            Q_ASSERT(defaultPluginBaseClass < pluginBaseClassCount);
            for (int i = 0; i < pluginBaseClassCount; i++)
                baseClasses.push_back(QLatin1String(pluginBaseClasses[i].name));
            m_filesPage->setBaseClassChoices(baseClasses);
            m_filesPage->setBaseClassName(baseClasses.at(defaultPluginBaseClass));
            m_pluginBaseClassesInitialized = true;
        }
        m_filesPage->setBaseClassInputVisible(true);
        break;
    default: {
        // Urrm, figure out a good class name. Use project name this time
        QString className = projectName();
        if (!className.isEmpty())
            className[0] = className.at(0).toUpper();
        m_filesPage->setClassName(className);
        m_filesPage->setBaseClassInputVisible(false);
    }
        break;
    }
}

void LibraryWizardDialog::setupMobilePage()
{
    m_mobilePage->setSymbianUid(AbstractMobileApp::symbianUidForPath(path()+projectName()));
    m_mobilePage->setLibraryType(type());
}

LibraryParameters LibraryWizardDialog::libraryParameters() const
{
    LibraryParameters rc;
    rc.className = m_filesPage->className();
    rc.baseClassName = m_filesPage->baseClassName();
    rc.sourceFileName = m_filesPage->sourceFileName();
    rc.headerFileName = m_filesPage->headerFileName();
    if (!rc.baseClassName.isEmpty())
        if (const PluginBaseClasses *plb = findPluginBaseClass(rc.baseClassName)) {
            rc.baseClassModule = QLatin1String(plb->module);
        }
    return rc;
}

MobileLibraryParameters LibraryWizardDialog::mobileLibraryParameters() const
{
    MobileLibraryParameters mlp;
    mlp.libraryType = type();
    mlp.fileName = projectName();

    //Symbian and Maemo stuff should always be added to pro file. Even if no mobile target is specified
    mlp.type |= MobileLibraryParameters::Symbian|MobileLibraryParameters::Maemo;

    if (mlp.type & MobileLibraryParameters::Symbian) {
        mlp.symbianUid = m_mobilePage->symbianUid();
        mlp.symbianCapabilities |= m_mobilePage->networkEnabled()?MobileLibraryParameters::NetworkServices:0;
    }

    return mlp;
}

} // namespace Internal
} // namespace Qt4ProjectManager
