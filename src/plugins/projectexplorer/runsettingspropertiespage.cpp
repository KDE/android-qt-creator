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

#include "runsettingspropertiespage.h"

#include "buildstepspage.h"
#include "deployconfiguration.h"
#include "deployconfigurationmodel.h"
#include "runconfigurationmodel.h"
#include "runconfiguration.h"
#include "target.h"
#include "project.h"

#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QPair>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>

namespace ProjectExplorer {
namespace Internal {

struct FactoryAndId
{
    ProjectExplorer::IRunConfigurationFactory *factory;
    QString id;
};


} // namespace Internal
} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Internal::FactoryAndId)

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using ExtensionSystem::PluginManager;

///
/// RunSettingsPanelFactory
///

QString RunSettingsPanelFactory::id() const
{
    return QLatin1String(RUNSETTINGS_PANEL_ID);
}

QString RunSettingsPanelFactory::displayName() const
{
    return RunSettingsWidget::tr("Run Settings");
}

bool RunSettingsPanelFactory::supports(Target *target)
{
    Q_UNUSED(target);
    return true;
}

PropertiesPanel *RunSettingsPanelFactory::createPanel(Target *target)
{
    PropertiesPanel *panel = new PropertiesPanel;
    QWidget *w = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(w);
    QWidget *b = new RunSettingsWidget(target);
    l->addWidget(b);
    l->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    l->setContentsMargins(QMargins());
    panel->setWidget(w);
    panel->setIcon(QIcon(QLatin1String(":/projectexplorer/images/RunSettings.png")));
    panel->setDisplayName(RunSettingsWidget::tr("Run Settings"));
    return panel;
}

///
/// RunSettingsWidget
///

RunSettingsWidget::RunSettingsWidget(Target *target)
    : m_target(target),
      m_runConfigurationsModel(new RunConfigurationModel(target, this)),
      m_deployConfigurationModel(new DeployConfigurationModel(target, this)),
      m_runConfigurationWidget(0),
      m_runLayout(0),
      m_deployConfigurationWidget(0),
      m_deployLayout(0),
      m_deploySteps(0),
      m_ignoreChange(false)
{
    Q_ASSERT(m_target);

    m_deployConfigurationCombo = new QComboBox(this);
    m_addDeployToolButton = new QPushButton(tr("Add"), this);
    m_removeDeployToolButton = new QPushButton(tr("Remove"), this);
    m_renameDeployButton = new QPushButton(tr("Rename..."), this);

    QWidget *deployWidget = new QWidget(this);

    m_runConfigurationCombo = new QComboBox(this);
    m_runConfigurationCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_runConfigurationCombo->setMinimumContentsLength(15);

    m_addRunToolButton = new QPushButton(tr("Add"), this);
    m_removeRunToolButton = new QPushButton(tr("Remove"), this);
    m_renameRunButton = new QPushButton(tr("Rename..."), this);

    QSpacerItem *spacer1 =
        new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
    QSpacerItem *spacer2 =
        new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

    QWidget *runWidget = new QWidget(this);

    QLabel *deployTitle = new QLabel(tr("Deployment"), this);
    QLabel *deployLabel = new QLabel(tr("Method:"), this);
    QLabel *runTitle = new QLabel(tr("Run"), this);
    QLabel *runLabel = new QLabel(tr("Run configuration:"), this);

    runLabel->setBuddy(m_runConfigurationCombo);

    QFont f = runLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);

    runTitle->setFont(f);
    deployTitle->setFont(f);

    m_gridLayout = new QGridLayout(this);
    m_gridLayout->setContentsMargins(0, 20, 0, 0);
    m_gridLayout->setHorizontalSpacing(6);
    m_gridLayout->setVerticalSpacing(8);
    m_gridLayout->addWidget(deployTitle, 0, 0, 1, 6);
    m_gridLayout->addWidget(deployLabel, 1, 0, 1, 1);
    m_gridLayout->addWidget(m_deployConfigurationCombo, 1, 1, 1, 1);
    m_gridLayout->addWidget(m_addDeployToolButton, 1, 2, 1, 1);
    m_gridLayout->addWidget(m_removeDeployToolButton, 1, 3, 1, 1);
    m_gridLayout->addWidget(m_renameDeployButton, 1, 4, 1, 1);
    m_gridLayout->addWidget(deployWidget, 2, 0, 1, 6);

    m_gridLayout->addWidget(runTitle, 3, 0, 1, 6);
    m_gridLayout->addWidget(runLabel, 4, 0, 1, 1);
    m_gridLayout->addWidget(m_runConfigurationCombo, 4, 1, 1, 1);
    m_gridLayout->addWidget(m_addRunToolButton, 4, 2, 1, 1);
    m_gridLayout->addWidget(m_removeRunToolButton, 4, 3, 1, 1);
    m_gridLayout->addWidget(m_renameRunButton, 4, 4, 1, 1);
    m_gridLayout->addItem(spacer1, 4, 5, 1, 1);
    m_gridLayout->addWidget(runWidget, 5, 0, 1, 6);
    m_gridLayout->addItem(spacer2, 6, 0, 1, 1);

    // deploy part
    deployWidget->setContentsMargins(0, 10, 0, 25);
    m_deployLayout = new QVBoxLayout(deployWidget);
    m_deployLayout->setMargin(0);
    m_deployLayout->setSpacing(5);

    m_deployConfigurationCombo->setModel(m_deployConfigurationModel);

    m_addDeployMenu = new QMenu(m_addDeployToolButton);
    m_addDeployToolButton->setMenu(m_addDeployMenu);

    updateDeployConfiguration(m_target->activeDeployConfiguration());

    // Some projects may not support deployment, so we need this:
    m_addDeployToolButton->setEnabled(m_target->activeDeployConfiguration());
    m_deployConfigurationCombo->setEnabled(m_target->activeDeployConfiguration());

    m_removeDeployToolButton->setEnabled(m_target->deployConfigurations().count() > 1);
    m_renameDeployButton->setEnabled(m_target->activeDeployConfiguration());

    connect(m_addDeployMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowDeployMenu()));
    connect(m_deployConfigurationCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentDeployConfigurationChanged(int)));
    connect(m_removeDeployToolButton, SIGNAL(clicked(bool)),
            this, SLOT(removeDeployConfiguration()));
    connect(m_renameDeployButton, SIGNAL(clicked()),
            this, SLOT(renameDeployConfiguration()));

    connect(m_target, SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
            this, SLOT(activeDeployConfigurationChanged()));

    // run part
    runWidget->setContentsMargins(0, 10, 0, 25);
    m_runLayout = new QVBoxLayout(runWidget);
    m_runLayout->setMargin(0);
    m_runLayout->setSpacing(5);

    m_addRunMenu = new QMenu(m_addRunToolButton);
    m_addRunToolButton->setMenu(m_addRunMenu);
    m_runConfigurationCombo->setModel(m_runConfigurationsModel);
    m_runConfigurationCombo->setCurrentIndex(
            m_runConfigurationsModel->indexFor(m_target->activeRunConfiguration()).row());

    m_removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
    m_renameRunButton->setEnabled(m_target->activeRunConfiguration());

    setConfigurationWidget(m_target->activeRunConfiguration());

    connect(m_addRunMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowAddMenu()));
    connect(m_runConfigurationCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentRunConfigurationChanged(int)));
    connect(m_removeRunToolButton, SIGNAL(clicked(bool)),
            this, SLOT(removeRunConfiguration()));
    connect(m_renameRunButton, SIGNAL(clicked()),
            this, SLOT(renameRunConfiguration()));

    connect(m_target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            this, SLOT(activeRunConfigurationChanged()));
}

RunSettingsWidget::~RunSettingsWidget()
{
}


void RunSettingsWidget::aboutToShowAddMenu()
{
    m_addRunMenu->clear();
    QList<IRunConfigurationFactory *> factories =
        ExtensionSystem::PluginManager::instance()->getObjects<IRunConfigurationFactory>();
    foreach (IRunConfigurationFactory *factory, factories) {
        QStringList ids = factory->availableCreationIds(m_target);
        foreach (const QString &id, ids) {
            QAction *action = m_addRunMenu->addAction(factory->displayNameForId(id));;
            FactoryAndId fai;
            fai.factory = factory;
            fai.id = id;
            QVariant v;
            v.setValue(fai);
            action->setData(v);
            connect(action, SIGNAL(triggered()),
                    this, SLOT(addRunConfiguration()));
        }
    }
}

void RunSettingsWidget::addRunConfiguration()
{
    QAction *act = qobject_cast<QAction *>(sender());
    if (!act)
        return;
    FactoryAndId fai = act->data().value<FactoryAndId>();
    RunConfiguration *newRC = fai.factory->create(m_target, fai.id);
    if (!newRC)
        return;
    m_target->addRunConfiguration(newRC);
    m_target->setActiveRunConfiguration(newRC);
    m_removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
}

void RunSettingsWidget::removeRunConfiguration()
{
    RunConfiguration *rc = m_target->activeRunConfiguration();
    QMessageBox msgBox(QMessageBox::Question, tr("Remove Run Configuration?"),
                       tr("Do you really want to delete the run configuration <b>%1</b>?").arg(rc->displayName()),
                       QMessageBox::Yes|QMessageBox::No, this);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setEscapeButton(QMessageBox::No);
    if (msgBox.exec() == QMessageBox::No)
        return;

    m_target->removeRunConfiguration(rc);
    m_removeRunToolButton->setEnabled(m_target->runConfigurations().size() > 1);
    m_renameRunButton->setEnabled(m_target->activeRunConfiguration());
}

void RunSettingsWidget::activeRunConfigurationChanged()
{
    if (m_ignoreChange)
        return;
    QModelIndex actRc = m_runConfigurationsModel->indexFor(m_target->activeRunConfiguration());
    m_ignoreChange = true;
    m_runConfigurationCombo->setCurrentIndex(actRc.row());
    setConfigurationWidget(m_runConfigurationsModel->runConfigurationAt(actRc.row()));
    m_ignoreChange = false;
}

void RunSettingsWidget::renameRunConfiguration()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Rename..."),
                                         tr("New name for run configuration <b>%1</b>:").
                                            arg(m_target->activeRunConfiguration()->displayName()),
                                         QLineEdit::Normal,
                                         m_target->activeRunConfiguration()->displayName(), &ok);
    if (!ok)
        return;

    name = uniqueRCName(name);
    if (name.isEmpty())
        return;

    m_target->activeRunConfiguration()->setDisplayName(name);
}

void RunSettingsWidget::currentRunConfigurationChanged(int index)
{
    if (m_ignoreChange)
        return;

    RunConfiguration *selectedRunConfiguration = 0;
    if (index >= 0)
        selectedRunConfiguration = m_runConfigurationsModel->runConfigurationAt(index);

    m_ignoreChange = true;
    m_target->setActiveRunConfiguration(selectedRunConfiguration);
    m_ignoreChange = false;

    // Update the run configuration configuration widget
    setConfigurationWidget(selectedRunConfiguration);
}

void RunSettingsWidget::currentDeployConfigurationChanged(int index)
{
    if (index == -1)
        updateDeployConfiguration(0);
    else
        m_target->setActiveDeployConfiguration(m_deployConfigurationModel->deployConfigurationAt(index));
}

void RunSettingsWidget::aboutToShowDeployMenu()
{
    m_addDeployMenu->clear();
    QStringList ids = m_target->availableDeployConfigurationIds();
    foreach (const QString &id, ids) {
        QAction *action = m_addDeployMenu->addAction(m_target->displayNameForDeployConfigurationId(id));
        action->setData(QVariant(id));
        connect(action, SIGNAL(triggered()),
                this, SLOT(addDeployConfiguration()));
    }
}

void RunSettingsWidget::addDeployConfiguration()
{
    QAction *act = qobject_cast<QAction *>(sender());
    if (!act)
        return;
    QString id = act->data().toString();
    DeployConfiguration *newDc = m_target->createDeployConfiguration(id);
    if (!newDc)
        return;
    m_target->addDeployConfiguration(newDc);
    m_target->setActiveDeployConfiguration(newDc);
    m_removeDeployToolButton->setEnabled(m_target->deployConfigurations().size() > 1);
}

void RunSettingsWidget::removeDeployConfiguration()
{
    DeployConfiguration *dc = m_target->activeDeployConfiguration();
    ProjectExplorer::BuildManager *bm = ProjectExplorerPlugin::instance()->buildManager();
    if (bm->isBuilding(dc)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(tr("Cancel Build && Remove Deploy Configuration"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(tr("Do Not Remove"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(tr("Remove Deploy Configuration %1?").arg(dc->displayName()));
        box.setText(tr("The deploy configuration <b>%1</b> is currently being built.").arg(dc->displayName()));
        box.setInformativeText(tr("Do you want to cancel the build process and remove the Deploy Configuration anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        bm->cancel();
    } else {
        QMessageBox msgBox(QMessageBox::Question, tr("Remove Deploy Configuration?"),
                           tr("Do you really want to delete deploy configuration <b>%1</b>?").arg(dc->displayName()),
                           QMessageBox::Yes|QMessageBox::No, this);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;
    }

    m_target->removeDeployConfiguration(dc);

    m_removeDeployToolButton->setEnabled(m_target->deployConfigurations().size() > 1);
}

void RunSettingsWidget::activeDeployConfigurationChanged()
{
    updateDeployConfiguration(m_target->activeDeployConfiguration());
}

void RunSettingsWidget::renameDeployConfiguration()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Rename..."),
                                         tr("New name for deploy configuration <b>%1</b>:").
                                            arg(m_target->activeDeployConfiguration()->displayName()),
                                         QLineEdit::Normal,
                                         m_target->activeDeployConfiguration()->displayName(), &ok);
    if (!ok)
        return;

    name = uniqueDCName(name);
    if (name.isEmpty())
        return;
    m_target->activeDeployConfiguration()->setDisplayName(name);
}

void RunSettingsWidget::updateDeployConfiguration(DeployConfiguration *dc)
{
    delete m_deployConfigurationWidget;
    m_deployConfigurationWidget = 0;
    delete m_deploySteps;
    m_deploySteps = 0;

    m_deployConfigurationCombo->setCurrentIndex(-1);

    if (!dc)
        return;

    QModelIndex actDc = m_deployConfigurationModel->indexFor(dc);
    m_deployConfigurationCombo->setCurrentIndex(actDc.row());

    m_deployConfigurationWidget = dc->configurationWidget();
    if (m_deployConfigurationWidget) {
        m_deployConfigurationWidget->init(dc);
        m_deployLayout->addWidget(m_deployConfigurationWidget);
    }

    m_deploySteps = new BuildStepListWidget;
    m_deploySteps->init(dc->stepList());
    m_deployLayout->addWidget(m_deploySteps);
}

void RunSettingsWidget::setConfigurationWidget(RunConfiguration *rc)
{
    delete m_runConfigurationWidget;
    m_runConfigurationWidget = 0;
    removeSubWidgets();
    if (!rc)
        return;
    m_runConfigurationWidget = rc->createConfigurationWidget();
    if (m_runConfigurationWidget)
        m_runLayout->addWidget(m_runConfigurationWidget);

    addRunControlWidgets();
}

QString RunSettingsWidget::uniqueDCName(const QString &name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList dcNames;
        foreach (DeployConfiguration *dc, m_target->deployConfigurations()) {
            if (dc == m_target->activeDeployConfiguration())
                continue;
            dcNames.append(dc->displayName());
        }
        result = Project::makeUnique(result, dcNames);
    }
    return result;
}

QString RunSettingsWidget::uniqueRCName(const QString &name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList rcNames;
        foreach (RunConfiguration *rc, m_target->runConfigurations()) {
            if (rc == m_target->activeRunConfiguration())
                continue;
            rcNames.append(rc->displayName());
        }
        result = Project::makeUnique(result, rcNames);
    }
    return result;
}

void RunSettingsWidget::addRunControlWidgets()
{
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    foreach (IRunControlFactory *f, pm->getObjects<IRunControlFactory>()) {
        ProjectExplorer::RunConfigWidget *rcw =
            f->createConfigurationWidget(m_target->activeRunConfiguration());
        if (rcw)
            addSubWidget(rcw);
    }
}

void RunSettingsWidget::addSubWidget(RunConfigWidget *widget)
{
    widget->setContentsMargins(0, 10, 0, 0);

    QLabel *label = new QLabel(this);
    label->setText(widget->displayName());
    connect(widget, SIGNAL(displayNameChanged(QString)),
            label, SLOT(setText(QString)));
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);
    label->setFont(f);

    label->setContentsMargins(0, 10, 0, 0);

    QGridLayout *l = m_gridLayout;
    l->addWidget(label, l->rowCount(), 0, 1, -1);
    l->addWidget(widget, l->rowCount(), 0, 1, -1);

    m_subWidgets.append(qMakePair(widget, label));
}

void RunSettingsWidget::removeSubWidgets()
{
    // foreach does not like commas in types, it's only a macro after all
    foreach (const RunConfigItem &item, m_subWidgets) {
        delete item.first;
        delete item.second;
    }
    m_subWidgets.clear();
}
