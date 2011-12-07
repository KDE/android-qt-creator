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
#include "remotelinuxrunconfigurationwidget.h"

#include "linuxdeviceconfiguration.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxenvironmentreader.h"
#include "remotelinuxsettingspages.h"
#include "remotelinuxutils.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <projectexplorer/environmentwidget.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>
#include <utils/detailswidget.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace Internal {
namespace {
const QString FetchEnvButtonText
    = QCoreApplication::translate("RemoteLinux::RemoteLinuxRunConfigurationWidget",
          "Fetch Device Environment");
} // anonymous namespace

class RemoteLinuxRunConfigurationWidgetPrivate
{
public:
    RemoteLinuxRunConfigurationWidgetPrivate(RemoteLinuxRunConfiguration *runConfig)
        : runConfiguration(runConfig), deviceEnvReader(runConfiguration), ignoreChange(false)
    {
    }

    RemoteLinuxRunConfiguration * const runConfiguration;
    RemoteLinuxEnvironmentReader deviceEnvReader;
    bool ignoreChange;

    QWidget topWidget;
    QLabel disabledIcon;
    QLabel disabledReason;
    QLineEdit argsLineEdit;
    QLineEdit workingDirLineEdit;
    QLabel localExecutableLabel;
    QLabel remoteExecutableLabel;
    QCheckBox useAlternateCommandBox;
    QLineEdit alternateCommand;
    QLabel devConfLabel;
    QLabel debuggingLanguagesLabel;
    QCheckBox debugCppButton;
    QCheckBox debugQmlButton;
    QPushButton fetchEnvButton;
    QComboBox baseEnvironmentComboBox;
    ProjectExplorer::EnvironmentWidget *environmentWidget;
    QFormLayout genericWidgetsLayout;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxRunConfigurationWidget::RemoteLinuxRunConfigurationWidget(RemoteLinuxRunConfiguration *runConfiguration,
        QWidget *parent)
    : QWidget(parent), d(new RemoteLinuxRunConfigurationWidgetPrivate(runConfiguration))
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    addDisabledLabel(topLayout);
    topLayout->addWidget(&d->topWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(&d->topWidget);
    mainLayout->setMargin(0);
    addGenericWidgets(mainLayout);
    addEnvironmentWidgets(mainLayout);

    connect(d->runConfiguration, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
        SLOT(handleCurrentDeviceConfigChanged()));
    handleCurrentDeviceConfigChanged();
    connect(d->runConfiguration, SIGNAL(isEnabledChanged(bool)),
        SLOT(runConfigurationEnabledChange(bool)));
    runConfigurationEnabledChange(d->runConfiguration->isEnabled());
}

RemoteLinuxRunConfigurationWidget::~RemoteLinuxRunConfigurationWidget()
{
    delete d;
}

void RemoteLinuxRunConfigurationWidget::addFormLayoutRow(QWidget *label, QWidget *field)
{
    d->genericWidgetsLayout.addRow(label, field);
}

void RemoteLinuxRunConfigurationWidget::addDisabledLabel(QVBoxLayout *topLayout)
{
    QHBoxLayout * const hl = new QHBoxLayout;
    hl->addStretch();
    d->disabledIcon.setPixmap(QPixmap(QString::fromUtf8(":/projectexplorer/images/compile_warning.png")));
    hl->addWidget(&d->disabledIcon);
    d->disabledReason.setVisible(false);
    hl->addWidget(&d->disabledReason);
    hl->addStretch();
    topLayout->addLayout(hl);
}

void RemoteLinuxRunConfigurationWidget::suppressQmlDebuggingOptions()
{
    d->debuggingLanguagesLabel.hide();
    d->debugCppButton.hide();
    d->debugQmlButton.hide();
}

void RemoteLinuxRunConfigurationWidget::runConfigurationEnabledChange(bool enabled)
{
    d->topWidget.setEnabled(enabled);
    d->disabledIcon.setVisible(!enabled);
    d->disabledReason.setVisible(!enabled);
    d->disabledReason.setText(d->runConfiguration->disabledReason());
}

void RemoteLinuxRunConfigurationWidget::addGenericWidgets(QVBoxLayout *mainLayout)
{
    Utils::DetailsWidget *detailsContainer = new Utils::DetailsWidget(this);
    detailsContainer->setState(Utils::DetailsWidget::NoSummary);

    QWidget *details = new QWidget(this);
    details->setLayout(&d->genericWidgetsLayout);
    detailsContainer->setWidget(details);

    mainLayout->addWidget(detailsContainer);

    d->genericWidgetsLayout.setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QWidget * const devConfWidget = new QWidget;
    QHBoxLayout * const devConfLayout = new QHBoxLayout(devConfWidget);
    devConfLayout->setMargin(0);
    devConfLayout->addWidget(&d->devConfLabel);
    QLabel * const addDevConfLabel= new QLabel(tr("<a href=\"%1\">Manage device configurations</a>")
        .arg(QLatin1String("deviceconfig")));
    addDevConfLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    devConfLayout->addWidget(addDevConfLabel);

    QLabel * const debuggerConfLabel = new QLabel(tr("<a href=\"%1\">Set Debugger</a>")
        .arg(QLatin1String("debugger")));
    debuggerConfLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    devConfLayout->addWidget(debuggerConfLabel);

    d->genericWidgetsLayout.addRow(new QLabel(tr("Device configuration:")), devConfWidget);
    d->localExecutableLabel.setText(d->runConfiguration->localExecutableFilePath());
    d->genericWidgetsLayout.addRow(tr("Executable on host:"), &d->localExecutableLabel);
    d->genericWidgetsLayout.addRow(tr("Executable on device:"), &d->remoteExecutableLabel);
    QWidget * const altRemoteExeWidget = new QWidget;
    QHBoxLayout * const altRemoteExeLayout = new QHBoxLayout(altRemoteExeWidget);
    altRemoteExeLayout->setContentsMargins(0, 0, 0, 0);
    d->alternateCommand.setText(d->runConfiguration->alternateRemoteExecutable());
    altRemoteExeLayout->addWidget(&d->alternateCommand);
    d->useAlternateCommandBox.setText(tr("Use this command instead"));
    d->useAlternateCommandBox.setChecked(d->runConfiguration->useAlternateExecutable());
    altRemoteExeLayout->addWidget(&d->useAlternateCommandBox);
    d->genericWidgetsLayout.addRow(tr("Alternate executable on device:"), altRemoteExeWidget);

    d->argsLineEdit.setText(d->runConfiguration->arguments());
    d->genericWidgetsLayout.addRow(tr("Arguments:"), &d->argsLineEdit);

    d->workingDirLineEdit.setPlaceholderText(tr("<default>"));
    d->workingDirLineEdit.setText(d->runConfiguration->workingDirectory());
    d->genericWidgetsLayout.addRow(tr("Working directory:"), &d->workingDirLineEdit);

    QHBoxLayout * const debugButtonsLayout = new QHBoxLayout;
    d->debugCppButton.setText(tr("C++"));
    d->debugQmlButton.setText(tr("QML"));
    d->debuggingLanguagesLabel.setText(tr("Debugging type:"));
    debugButtonsLayout->addWidget(&d->debugCppButton);
    debugButtonsLayout->addWidget(&d->debugQmlButton);
    debugButtonsLayout->addStretch(1);
    d->genericWidgetsLayout.addRow(&d->debuggingLanguagesLabel, debugButtonsLayout);
    d->debugCppButton.setChecked(d->runConfiguration->useCppDebugger());
    d->debugQmlButton.setChecked(d->runConfiguration->useQmlDebugger());

    connect(addDevConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showDeviceConfigurationsDialog(QString)));
    connect(debuggerConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showDeviceConfigurationsDialog(QString)));
    connect(&d->argsLineEdit, SIGNAL(textEdited(QString)), SLOT(argumentsEdited(QString)));
    connect(&d->debugCppButton, SIGNAL(toggled(bool)), SLOT(handleDebuggingTypeChanged()));
    connect(&d->debugQmlButton, SIGNAL(toggled(bool)), SLOT(handleDebuggingTypeChanged()));
    connect(d->runConfiguration, SIGNAL(targetInformationChanged()), this,
        SLOT(updateTargetInformation()));
    connect(d->runConfiguration, SIGNAL(deploySpecsChanged()), SLOT(handleDeploySpecsChanged()));
    connect(&d->useAlternateCommandBox, SIGNAL(toggled(bool)),
        SLOT(handleUseAlternateCommandChanged()));
    connect(&d->alternateCommand, SIGNAL(textEdited(QString)),
        SLOT(handleAlternateCommandChanged()));
    connect(&d->workingDirLineEdit, SIGNAL(textEdited(QString)),
        SLOT(handleWorkingDirectoryChanged()));
    handleDeploySpecsChanged();
    handleUseAlternateCommandChanged();
}

void RemoteLinuxRunConfigurationWidget::addEnvironmentWidgets(QVBoxLayout *mainLayout)
{
    QWidget * const baseEnvironmentWidget = new QWidget;
    QHBoxLayout * const baseEnvironmentLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseEnvironmentLayout->setMargin(0);
    QLabel * const label = new QLabel(tr("Base environment for this run configuration:"), this);
    baseEnvironmentLayout->addWidget(label);
    d->baseEnvironmentComboBox.addItems(QStringList() << tr("Clean Environment")
        << tr("System Environment"));
    d->baseEnvironmentComboBox.setCurrentIndex(d->runConfiguration->baseEnvironmentType());
    baseEnvironmentLayout->addWidget(&d->baseEnvironmentComboBox);

    d->fetchEnvButton.setText(FetchEnvButtonText);
    baseEnvironmentLayout->addWidget(&d->fetchEnvButton);
    baseEnvironmentLayout->addStretch(10);

    d->environmentWidget = new ProjectExplorer::EnvironmentWidget(this, baseEnvironmentWidget);
    d->environmentWidget->setBaseEnvironment(d->deviceEnvReader.remoteEnvironment());
    d->environmentWidget->setBaseEnvironmentText(d->runConfiguration->baseEnvironmentText());
    d->environmentWidget->setUserChanges(d->runConfiguration->userEnvironmentChanges());
    mainLayout->addWidget(d->environmentWidget);

    connect(d->environmentWidget, SIGNAL(userChangesChanged()), SLOT(userChangesEdited()));
    connect(&d->baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(baseEnvironmentSelected(int)));
    connect(d->runConfiguration, SIGNAL(baseEnvironmentChanged()),
        this, SLOT(baseEnvironmentChanged()));
    connect(d->runConfiguration, SIGNAL(remoteEnvironmentChanged()),
        this, SLOT(remoteEnvironmentChanged()));
    connect(d->runConfiguration,
        SIGNAL(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)),
        SLOT(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)));
    connect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(&d->deviceEnvReader, SIGNAL(finished()), this, SLOT(fetchEnvironmentFinished()));
    connect(&d->deviceEnvReader, SIGNAL(error(QString)), SLOT(fetchEnvironmentError(QString)));
}

void RemoteLinuxRunConfigurationWidget::argumentsEdited(const QString &text)
{
    d->runConfiguration->setArguments(text);
}

void RemoteLinuxRunConfigurationWidget::updateTargetInformation()
{
    d->localExecutableLabel
        .setText(QDir::toNativeSeparators(d->runConfiguration->localExecutableFilePath()));
}

void RemoteLinuxRunConfigurationWidget::handleDeploySpecsChanged()
{
    d->remoteExecutableLabel.setText(d->runConfiguration->defaultRemoteExecutableFilePath());
}

void RemoteLinuxRunConfigurationWidget::handleUseAlternateCommandChanged()
{
    const bool useAltExe = d->useAlternateCommandBox.isChecked();
    d->remoteExecutableLabel.setEnabled(!useAltExe);
    d->alternateCommand.setEnabled(useAltExe);
    d->runConfiguration->setUseAlternateExecutable(useAltExe);
}

void RemoteLinuxRunConfigurationWidget::handleAlternateCommandChanged()
{
    d->runConfiguration->setAlternateRemoteExecutable(d->alternateCommand.text().trimmed());
}

void RemoteLinuxRunConfigurationWidget::handleWorkingDirectoryChanged()
{
    d->runConfiguration->setWorkingDirectory(d->workingDirLineEdit.text().trimmed());
}

void RemoteLinuxRunConfigurationWidget::showDeviceConfigurationsDialog(const QString &link)
{
    if (link == QLatin1String("deviceconfig")) {
        Core::ICore::instance()->showOptionsDialog(LinuxDeviceConfigurationsSettingsPage::pageCategory(),
            LinuxDeviceConfigurationsSettingsPage::pageId());
    } else if (link == QLatin1String("debugger")) {
        Core::ICore::instance()->showOptionsDialog(QLatin1String("O.Debugger"),
            QLatin1String("M.Gdb"));
    }
}

void RemoteLinuxRunConfigurationWidget::handleCurrentDeviceConfigChanged()
{
    d->devConfLabel.setText(RemoteLinuxUtils::deviceConfigurationName(d->runConfiguration->deviceConfig()));
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironment()
{
    disconnect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    d->fetchEnvButton.setText(tr("Cancel Fetch Operation"));
    d->deviceEnvReader.start(d->runConfiguration->environmentPreparationCommand());
}

void RemoteLinuxRunConfigurationWidget::stopFetchEnvironment()
{
    d->deviceEnvReader.stop();
    fetchEnvironmentFinished();
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironmentFinished()
{
    disconnect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    connect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    d->fetchEnvButton.setText(FetchEnvButtonText);
    d->runConfiguration->setRemoteEnvironment(d->deviceEnvReader.remoteEnvironment());
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironmentError(const QString &error)
{
    QMessageBox::warning(this, tr("Device Error"),
        tr("Fetching environment failed: %1").arg(error));
}

void RemoteLinuxRunConfigurationWidget::userChangesEdited()
{
    d->ignoreChange = true;
    d->runConfiguration->setUserEnvironmentChanges(d->environmentWidget->userChanges());
    d->ignoreChange = false;
}

void RemoteLinuxRunConfigurationWidget::baseEnvironmentSelected(int index)
{
    d->ignoreChange = true;
    d->runConfiguration->setBaseEnvironmentType(RemoteLinuxRunConfiguration::BaseEnvironmentType(index));
    d->environmentWidget->setBaseEnvironment(d->runConfiguration->baseEnvironment());
    d->environmentWidget->setBaseEnvironmentText(d->runConfiguration->baseEnvironmentText());
    d->ignoreChange = false;
}

void RemoteLinuxRunConfigurationWidget::baseEnvironmentChanged()
{
    if (d->ignoreChange)
        return;

    d->baseEnvironmentComboBox.setCurrentIndex(d->runConfiguration->baseEnvironmentType());
    d->environmentWidget->setBaseEnvironment(d->runConfiguration->baseEnvironment());
    d->environmentWidget->setBaseEnvironmentText(d->runConfiguration->baseEnvironmentText());
}

void RemoteLinuxRunConfigurationWidget::remoteEnvironmentChanged()
{
    d->environmentWidget->setBaseEnvironment(d->runConfiguration->remoteEnvironment());
}

void RemoteLinuxRunConfigurationWidget::userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges)
{
    if (d->ignoreChange)
        return;
    d->environmentWidget->setUserChanges(userChanges);
}

void RemoteLinuxRunConfigurationWidget::handleDebuggingTypeChanged()
{
    d->runConfiguration->setUseCppDebugger(d->debugCppButton.isChecked());
    d->runConfiguration->setUseQmlDebugger(d->debugQmlButton.isChecked());
}

} // namespace RemoteLinux
