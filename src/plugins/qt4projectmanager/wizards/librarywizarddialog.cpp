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
#include <QtCore/QDir>

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

    if (m_targetPageId != -1)
        m_mobilePageId = addPage(m_mobilePage);

    m_modulesPageId = addModulesPage();

    m_filesPage->setNamespacesEnabled(true);
    m_filesPage->setFormFileInputVisible(false);
    m_filesPage->setClassTypeComboVisible(false);

    m_filesPageId = addPage(m_filesPage);

    Utils::WizardProgressItem *introItem = wizardProgress()->item(startId());
    Utils::WizardProgressItem *targetItem = 0;
    if (m_targetPageId != -1)
        targetItem = wizardProgress()->item(m_targetPageId);
    Utils::WizardProgressItem *mobileItem = wizardProgress()->item(m_mobilePageId);
    mobileItem->setTitle(QLatin1String("    ") + tr("Symbian Specific"));
    Utils::WizardProgressItem *modulesItem = wizardProgress()->item(m_modulesPageId);
    Utils::WizardProgressItem *filesItem = wizardProgress()->item(m_filesPageId);
    filesItem->setTitle(tr("Details"));

    if (m_targetPageId != -1) {
        targetItem->setNextItems(QList<Utils::WizardProgressItem *>()
                                 << mobileItem << modulesItem << filesItem);
        targetItem->setNextShownItem(0);
        mobileItem->setNextItems(QList<Utils::WizardProgressItem *>()
                                 << modulesItem << filesItem);
        mobileItem->setNextShownItem(0);
    } else {
        introItem->setNextItems(QList<Utils::WizardProgressItem *>()
                                << modulesItem << filesItem);
        introItem->setNextShownItem(0);
    }


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

bool LibraryWizardDialog::isModulesPageSkipped() const
{
    // When leaving the intro, target or mobile page, the modules page is skipped
    // in the case of a plugin since it knows its dependencies by itself.
    return type() == QtProjectParameters::Qt4Plugin;
}

int LibraryWizardDialog::skipModulesPageIfNeeded() const
{
    if (isModulesPageSkipped())
        return m_filesPageId;
    return m_modulesPageId;
}

int LibraryWizardDialog::nextId() const
{
    if (m_targetPageId != -1) {
        if (currentId() == m_targetPageId) {

            int next = m_modulesPageId;

            const bool symbianTargetEnabled = isTargetSelected(QLatin1String(Constants::S60_DEVICE_TARGET_ID))
                    || isTargetSelected(QLatin1String(Constants::S60_EMULATOR_TARGET_ID));

            // If there was no Symbian target defined we omit "Symbian specific" step
            // We also omit this step if the library type is not dll
            if (symbianTargetEnabled
                    && (type() == QtProjectParameters::SharedLibrary
                        || type() == QtProjectParameters::Qt4Plugin))
                next = m_mobilePageId;

            if (next == m_modulesPageId)
                return skipModulesPageIfNeeded();

            return next;
        } else if (currentId() == m_mobilePageId) {
            return skipModulesPageIfNeeded();
        }
    } else if (currentId() == startId()) {
        return skipModulesPageIfNeeded();
    }

    return BaseQt4ProjectWizardDialog::nextId();
}

void LibraryWizardDialog::initializePage(int id)
{
    if (m_targetPageId != -1 && id == m_targetPageId) {
        Utils::WizardProgressItem *mobileItem = wizardProgress()->item(m_mobilePageId);
        Utils::WizardProgressItem *modulesItem = wizardProgress()->item(m_modulesPageId);
        Utils::WizardProgressItem *filesItem = wizardProgress()->item(m_filesPageId);
        if (isModulesPageSkipped())
            mobileItem->setNextShownItem(filesItem);
        else
            mobileItem->setNextShownItem(modulesItem);

    }
    BaseQt4ProjectWizardDialog::initializePage(id);
}

void LibraryWizardDialog::cleanupPage(int id)
{
    if (m_targetPageId != -1 && id == m_targetPageId) {
        Utils::WizardProgressItem *mobileItem = wizardProgress()->item(m_mobilePageId);
        mobileItem->setNextShownItem(0);
    }
    BaseQt4ProjectWizardDialog::cleanupPage(id);
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

    if (type() == QtProjectParameters::Qt4Plugin)
        m_mobilePage->setQtPluginDirectory(projectName());
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
        mlp.qtPluginDirectory = m_mobilePage->qtPluginDirectory();
        mlp.symbianCapabilities |= m_mobilePage->networkEnabled()?MobileLibraryParameters::NetworkServices:0;
    }

    return mlp;
}

} // namespace Internal
} // namespace Qt4ProjectManager
