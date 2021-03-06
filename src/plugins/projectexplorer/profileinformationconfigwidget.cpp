/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "profileinformationconfigwidget.h"

#include "devicesupport/devicemanager.h"
#include "devicesupport/devicemanagermodel.h"
#include "devicesupport/idevicefactory.h"
#include "projectexplorerconstants.h"
#include "profile.h"
#include "profileinformation.h"
#include "toolchain.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// SysRootInformationConfigWidget:
// --------------------------------------------------------------------------

SysRootInformationConfigWidget::SysRootInformationConfigWidget(Profile *p, QWidget *parent) :
    ProfileConfigWidget(parent),
    m_profile(p)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    m_chooser = new Utils::PathChooser;
    m_chooser->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chooser);
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);

    m_chooser->setFileName(SysRootProfileInformation::sysRoot(p));

    connect(m_chooser, SIGNAL(changed(QString)), this, SIGNAL(dirty()));
}

QString SysRootInformationConfigWidget::displayName() const
{
    return tr("Sysroot:");
}

void SysRootInformationConfigWidget::apply()
{
    SysRootProfileInformation::setSysRoot(m_profile, m_chooser->fileName());
}

void SysRootInformationConfigWidget::discard()
{
    m_chooser->setFileName(SysRootProfileInformation::sysRoot(m_profile));
}

bool SysRootInformationConfigWidget::isDirty() const
{
    return SysRootProfileInformation::sysRoot(m_profile) != m_chooser->fileName();
}

void SysRootInformationConfigWidget::makeReadOnly()
{
    m_chooser->setEnabled(false);
}

QWidget *SysRootInformationConfigWidget::buttonWidget() const
{
    return m_chooser->buttonAtIndex(0);
}

// --------------------------------------------------------------------------
// ToolChainInformationConfigWidget:
// --------------------------------------------------------------------------

ToolChainInformationConfigWidget::ToolChainInformationConfigWidget(Profile *p, QWidget *parent) :
    ProfileConfigWidget(parent),
    m_isReadOnly(false), m_profile(p),
    m_comboBox(new QComboBox), m_manageButton(new QPushButton(this))
{
    ToolChainManager *tcm = ToolChainManager::instance();

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    m_comboBox->setContentsMargins(0, 0, 0, 0);
    m_comboBox->setEnabled(false);
    m_comboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    layout->addWidget(m_comboBox);

    foreach (ToolChain *tc, tcm->toolChains())
        toolChainAdded(tc);

    updateComboBox();

    discard();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dirty()));

    m_manageButton->setContentsMargins(0, 0, 0, 0);
    m_manageButton->setText(tr("Manage..."));
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageToolChains()));

    connect(tcm, SIGNAL(toolChainAdded(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainAdded(ProjectExplorer::ToolChain*)));
    connect(tcm, SIGNAL(toolChainRemoved(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainRemoved(ProjectExplorer::ToolChain*)));
    connect(tcm, SIGNAL(toolChainUpdated(ProjectExplorer::ToolChain*)),
            this, SLOT(toolChainUpdated(ProjectExplorer::ToolChain*)));
}

QString ToolChainInformationConfigWidget::displayName() const
{
    return tr("Tool chain:");
}

void ToolChainInformationConfigWidget::apply()
{
    const QString id = m_comboBox->itemData(m_comboBox->currentIndex()).toString();
    ToolChain *tc = ToolChainManager::instance()->findToolChain(id);
    ToolChainProfileInformation::setToolChain(m_profile, tc);
}

void ToolChainInformationConfigWidget::discard()
{
    m_comboBox->setCurrentIndex(indexOf(ToolChainProfileInformation::toolChain(m_profile)));
}

bool ToolChainInformationConfigWidget::isDirty() const
{
    ToolChain *tc = ToolChainProfileInformation::toolChain(m_profile);
    return (m_comboBox->itemData(m_comboBox->currentIndex()).toString())
            == (tc ? tc->id() : QString());
}

void ToolChainInformationConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

QWidget *ToolChainInformationConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

void ToolChainInformationConfigWidget::toolChainAdded(ProjectExplorer::ToolChain *tc)
{
    m_comboBox->addItem(tc->displayName(), tc->id());
    updateComboBox();
}

void ToolChainInformationConfigWidget::toolChainRemoved(ProjectExplorer::ToolChain *tc)
{
    const int pos = indexOf(tc);
    if (pos < 0)
        return;
    m_comboBox->removeItem(pos);
    updateComboBox();
}
void ToolChainInformationConfigWidget::toolChainUpdated(ProjectExplorer::ToolChain *tc)
{
    const int pos = indexOf(tc);
    if (pos < 0)
        return;
    m_comboBox->setItemText(pos, tc->displayName());
}

void ToolChainInformationConfigWidget::manageToolChains()
{
    Core::ICore::showOptionsDialog(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY),
                                   QLatin1String(ProjectExplorer::Constants::TOOLCHAIN_SETTINGS_PAGE_ID));
}

void ToolChainInformationConfigWidget::updateComboBox()
{
    // remove unavailable tool chain:
    int pos = indexOf(0);
    if (pos >= 0)
        m_comboBox->removeItem(pos);

    if (m_comboBox->count() == 0) {
        m_comboBox->addItem(tr("<No tool chain available>"), QString());
        m_comboBox->setEnabled(false);
    } else {
        m_comboBox->setEnabled(!m_isReadOnly);
    }
}

int ToolChainInformationConfigWidget::indexOf(const ToolChain *tc)
{
    const QString id = tc ? tc->id() : QString();
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (id == m_comboBox->itemData(i).toString())
            return i;
    }
    return -1;
}

// --------------------------------------------------------------------------
// DeviceTypeInformationConfigWidget:
// --------------------------------------------------------------------------

DeviceTypeInformationConfigWidget::DeviceTypeInformationConfigWidget(Profile *p, QWidget *parent) :
    ProfileConfigWidget(parent),
    m_isReadOnly(false), m_profile(p),
    m_comboBox(new QComboBox)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    m_comboBox->setContentsMargins(0, 0, 0, 0);
    m_comboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    layout->addWidget(m_comboBox);

    QList<IDeviceFactory *> factories
            = ExtensionSystem::PluginManager::instance()->getObjects<IDeviceFactory>();
    foreach (IDeviceFactory *factory, factories) {
        foreach (Core::Id id, factory->availableCreationIds()) {
            m_comboBox->addItem(factory->displayNameForId(id), QVariant::fromValue(id));
        }
    }

    discard();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dirty()));
}

QString DeviceTypeInformationConfigWidget::displayName() const
{
    return tr("Device Type:");
}

void DeviceTypeInformationConfigWidget::apply()
{
    Core::Id devType;
    if (m_comboBox->currentIndex() >= 0)
        devType = m_comboBox->itemData(m_comboBox->currentIndex()).value<Core::Id>();
    DeviceTypeProfileInformation::setDeviceTypeId(m_profile, devType);
}

void DeviceTypeInformationConfigWidget::discard()
{
    Core::Id devType = DeviceTypeProfileInformation::deviceTypeId(m_profile);
    if (!devType.isValid())
        m_comboBox->setCurrentIndex(-1);
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (m_comboBox->itemData(i).value<Core::Id>() == devType) {
            m_comboBox->setCurrentIndex(i);
            break;
        }
    }
}

bool DeviceTypeInformationConfigWidget::isDirty() const
{
    Core::Id devType;
    if (m_comboBox->currentIndex() >= 0)
        devType = m_comboBox->itemData(m_comboBox->currentIndex()).value<Core::Id>();
    return DeviceTypeProfileInformation::deviceTypeId(m_profile) != devType;
}

void DeviceTypeInformationConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

// --------------------------------------------------------------------------
// DeviceInformationConfigWidget:
// --------------------------------------------------------------------------

DeviceInformationConfigWidget::DeviceInformationConfigWidget(Profile *p, QWidget *parent) :
    ProfileConfigWidget(parent),
    m_isReadOnly(false), m_profile(p),
    m_comboBox(new QComboBox), m_manageButton(new QPushButton(this)),
    m_model(new DeviceManagerModel(DeviceManager::instance()))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    m_comboBox->setContentsMargins(0, 0, 0, 0);
    m_comboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    layout->addWidget(m_comboBox);

    m_comboBox->setModel(m_model);

    m_manageButton->setContentsMargins(0, 0, 0, 0);
    m_manageButton->setText(tr("Manage..."));

    discard();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dirty()));

    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageDevices()));
}

QString DeviceInformationConfigWidget::displayName() const
{
    return tr("Device:");
}

void DeviceInformationConfigWidget::apply()
{
    int idx = m_comboBox->currentIndex();
    if (idx >= 0)
        DeviceProfileInformation::setDeviceId(m_profile, m_model->deviceId(idx));
    else
        DeviceProfileInformation::setDeviceId(m_profile, IDevice::invalidId());
}

void DeviceInformationConfigWidget::discard()
{
    m_comboBox->setCurrentIndex(m_model->indexOf(DeviceProfileInformation::device(m_profile)));
}

bool DeviceInformationConfigWidget::isDirty() const
{
    Core::Id devId = DeviceProfileInformation::deviceId(m_profile);
    return devId != m_model->deviceId(m_comboBox->currentIndex());
}

void DeviceInformationConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

QWidget *DeviceInformationConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

void DeviceInformationConfigWidget::manageDevices()
{
    Core::ICore::showOptionsDialog(QLatin1String(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY),
                                   QLatin1String(ProjectExplorer::Constants::DEVICE_SETTINGS_PAGE_ID));
}

} // namespace Internal
} // namespace ProjectExplorer
