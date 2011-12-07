/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "maemorunconfigurationwidget.h"

#include "maemoglobal.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfiguration.h"
#include "qt4maemotarget.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <projectexplorer/environmentwidget.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>
#include <remotelinux/remotelinuxrunconfigurationwidget.h>
#include <utils/detailswidget.h>

#include <QtGui/QButtonGroup>
#include <QtCore/QCoreApplication>
#include <QtGui/QComboBox>
#include <QtGui/QFileDialog>
#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QTableView>
#include <QtGui/QToolButton>

using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

MaemoRunConfigurationWidget::MaemoRunConfigurationWidget(
        MaemoRunConfiguration *runConfiguration, QWidget *parent)
    : QWidget(parent), m_runConfiguration(runConfiguration)
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    QWidget *topWidget = new QWidget;
    topLayout->addWidget(topWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(topWidget);
    mainLayout->setMargin(0);
    m_remoteLinuxRunConfigWidget = new RemoteLinuxRunConfigurationWidget(runConfiguration, parent);
    mainLayout->addWidget(m_remoteLinuxRunConfigWidget);
    m_subWidget = new QWidget;
    mainLayout->addWidget(m_subWidget);
    QVBoxLayout *subLayout = new QVBoxLayout(m_subWidget);
    subLayout->setMargin(0);
    addMountWidgets(subLayout);
    connect(m_runConfiguration, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
        this, SLOT(updateMountWarning()));
    connect(m_runConfiguration, SIGNAL(debuggersChanged()), SLOT(updateMountWarning()));
    updateMountWarning();

    const AbstractQt4MaemoTarget * const maemoTarget
        = qobject_cast<AbstractQt4MaemoTarget *>(runConfiguration->target());
    m_mountDetailsContainer->setVisible(maemoTarget->allowsRemoteMounts());
    if (!maemoTarget->allowsQmlDebugging())
        m_remoteLinuxRunConfigWidget->suppressQmlDebuggingOptions();

    connect(m_runConfiguration, SIGNAL(isEnabledChanged(bool)),
            this, SLOT(runConfigurationEnabledChange(bool)));
    runConfigurationEnabledChange(m_runConfiguration->isEnabled());
}

void MaemoRunConfigurationWidget::runConfigurationEnabledChange(bool enabled)
{
    m_subWidget->setEnabled(enabled);
}

void MaemoRunConfigurationWidget::addMountWidgets(QVBoxLayout *mainLayout)
{
    m_mountDetailsContainer = new Utils::DetailsWidget(this);
    QWidget *mountViewWidget = new QWidget;
    m_mountDetailsContainer->setWidget(mountViewWidget);
    mainLayout->addWidget(m_mountDetailsContainer);
    QVBoxLayout *mountViewLayout = new QVBoxLayout(mountViewWidget);
    m_mountWarningLabel = new QLabel;
    mountViewLayout->addWidget(m_mountWarningLabel);
    QHBoxLayout *tableLayout = new QHBoxLayout;
    mountViewLayout->addLayout(tableLayout);
    m_mountView = new QTableView;
    m_mountView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    m_mountView->setSelectionBehavior(QTableView::SelectRows);
    m_mountView->setModel(m_runConfiguration->remoteMounts());
    tableLayout->addWidget(m_mountView);
    QVBoxLayout *mountViewButtonsLayout = new QVBoxLayout;
    tableLayout->addLayout(mountViewButtonsLayout);
    QToolButton *addMountButton = new QToolButton;
    QIcon plusIcon;
    plusIcon.addFile(QLatin1String(Core::Constants::ICON_PLUS));
    addMountButton->setIcon(plusIcon);
    mountViewButtonsLayout->addWidget(addMountButton);
    m_removeMountButton = new QToolButton;
    QIcon minusIcon;
    minusIcon.addFile(QLatin1String(Core::Constants::ICON_MINUS));
    m_removeMountButton->setIcon(minusIcon);
    mountViewButtonsLayout->addWidget(m_removeMountButton);
    mountViewButtonsLayout->addStretch(1);

    connect(addMountButton, SIGNAL(clicked()), this, SLOT(addMount()));
    connect(m_removeMountButton, SIGNAL(clicked()), this, SLOT(removeMount()));
    connect(m_mountView, SIGNAL(doubleClicked(QModelIndex)), this,
        SLOT(changeLocalMountDir(QModelIndex)));
    connect(m_mountView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
        SLOT(enableOrDisableRemoveMountSpecButton()));
    enableOrDisableRemoveMountSpecButton();
    connect(m_runConfiguration, SIGNAL(remoteMountsChanged()), SLOT(handleRemoteMountsChanged()));
    handleRemoteMountsChanged();
}

void MaemoRunConfigurationWidget::enableOrDisableRemoveMountSpecButton()
{
    const QModelIndexList selectedRows
        = m_mountView->selectionModel()->selectedRows();
    m_removeMountButton->setEnabled(!selectedRows.isEmpty());
}

void MaemoRunConfigurationWidget::addMount()
{
    const QString localDir = QFileDialog::getExistingDirectory(this,
        tr("Choose directory to mount"));
    if (!localDir.isEmpty()) {
        MaemoRemoteMountsModel * const mountsModel
            = m_runConfiguration->remoteMounts();
        mountsModel->addMountSpecification(localDir);
        m_mountView->edit(mountsModel->index(mountsModel->mountSpecificationCount() - 1,
            mountsModel->RemoteMountPointRow));
    }
}

void MaemoRunConfigurationWidget::removeMount()
{
    const QModelIndexList selectedRows
        = m_mountView->selectionModel()->selectedRows();
    if (!selectedRows.isEmpty()) {
        m_runConfiguration->remoteMounts()
            ->removeMountSpecificationAt(selectedRows.first().row());
    }
}

void MaemoRunConfigurationWidget::changeLocalMountDir(const QModelIndex &index)
{
    if (index.column() == MaemoRemoteMountsModel::LocalDirRow) {
        MaemoRemoteMountsModel * const mountsModel
            = m_runConfiguration->remoteMounts();
        const QString oldDir
            = mountsModel->mountSpecificationAt(index.row()).localDir;
        const QString localDir = QFileDialog::getExistingDirectory(this,
            tr("Choose directory to mount"), oldDir);
        if (!localDir.isEmpty())
            mountsModel->setLocalDir(index.row(), localDir);
    }
}

void MaemoRunConfigurationWidget::handleRemoteMountsChanged()
{
    const int mountCount
        = m_runConfiguration->remoteMounts()->validMountSpecificationCount();
    QString text;
    switch (mountCount) {
    case 0:
        text = tr("No local directories to be mounted on the device.");
        break;
    case 1:
        text = tr("One local directory to be mounted on the device.");
        break;
    default:
        //: Note: Only mountCount>1  will occur here as 0, 1 are handled above.
        text = tr("%n local directories to be mounted on the device.", 0, mountCount);
        break;
    }
    m_mountDetailsContainer->setSummaryText(QLatin1String("<b>") + text
        + QLatin1String("</b>"));
    updateMountWarning();
}

void MaemoRunConfigurationWidget::updateMountWarning()
{
    QString mountWarning;
    const PortList &portList = m_runConfiguration->freePorts();
    const int availablePortCount = portList.count();
    const int mountDirCount
            = m_runConfiguration->remoteMounts()->validMountSpecificationCount();
    if (mountDirCount > availablePortCount) {
        mountWarning = tr("WARNING: You want to mount %1 directories, but "
            "your device has only %n free ports.<br>You will not be able "
            "to run this configuration.", 0, availablePortCount)
                .arg(mountDirCount);
    } else if (mountDirCount > 0) {
        const int portsLeftByDebuggers = availablePortCount
                - m_runConfiguration->portsUsedByDebuggers();
        if (mountDirCount > portsLeftByDebuggers) {
            mountWarning = tr("WARNING: You want to mount %1 directories, "
                "but only %n ports on the device will be available "
                "in debug mode. <br>You will not be able to debug your "
                "application with this configuration.", 0, portsLeftByDebuggers)
                    .arg(mountDirCount);
        }
    }
    if (mountWarning.isEmpty()) {
        m_mountWarningLabel->hide();
    } else {
        m_mountWarningLabel->setText(QLatin1String("<font color=\"red\">")
            + mountWarning + QLatin1String("</font>"));
        m_mountWarningLabel->show();
        m_mountDetailsContainer->setState(Utils::DetailsWidget::Expanded);
    }
}

} // namespace Internal
} // namespace Madde
