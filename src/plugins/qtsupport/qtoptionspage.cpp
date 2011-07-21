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

#include "qtoptionspage.h"
#include "ui_showbuildlog.h"
#include "ui_qtversionmanager.h"
#include "ui_qtversioninfo.h"
#include "ui_debugginghelper.h"
#include "qtsupportconstants.h"
#include "qtversionmanager.h"
#include "qtversionfactory.h"
#include "qmldumptool.h"
#include "qmldebugginglibrary.h"
#include "qmlobservertool.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/treewidgetcolumnstretcher.h>
#include <utils/qtcassert.h>
#include <utils/buildablehelperlibrary.h>
#include <utils/pathchooser.h>
#include <projectexplorer/toolchainmanager.h>
#include <qtconcurrent/runextensions.h>

#include <QtCore/QDir>
#include <QtGui/QToolTip>
#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>
#include <QtGui/QMainWindow>

enum ModelRoles { VersionIdRole = Qt::UserRole, BuildLogRole, BuildRunningRole};

using namespace QtSupport;
using namespace QtSupport::Internal;

///
// QtOptionsPage
///

QtOptionsPage::QtOptionsPage()
    : m_widget(0)
{
}

QString QtOptionsPage::id() const
{
    return QLatin1String(Constants::QTVERSION_SETTINGS_PAGE_ID);
}

QString QtOptionsPage::displayName() const
{
    return QCoreApplication::translate("Qt4ProjectManager", Constants::QTVERSION_SETTINGS_PAGE_NAME);
}

QString QtOptionsPage::category() const
{
    return QLatin1String(Constants::QT_SETTINGS_CATEGORY);
}

QString QtOptionsPage::displayCategory() const
{
    return QCoreApplication::translate("Qt4ProjectManager", Constants::QT_SETTINGS_TR_CATEGORY);
}

QIcon QtOptionsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::QT_SETTINGS_CATEGORY_ICON));
}

QWidget *QtOptionsPage::createPage(QWidget *parent)
{
    QtVersionManager *vm = QtVersionManager::instance();
    m_widget = new QtOptionsPageWidget(parent, vm->versions());
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void QtOptionsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    m_widget->finish();

    QtVersionManager *vm = QtVersionManager::instance();
    vm->setNewQtVersions(m_widget->versions());
}

bool QtOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

//-----------------------------------------------------


QtOptionsPageWidget::QtOptionsPageWidget(QWidget *parent, QList<BaseQtVersion *> versions)
    : QWidget(parent)
    , m_specifyNameString(tr("<specify a name>"))
    , m_ui(new Internal::Ui::QtVersionManager())
    , m_versionUi(new Internal::Ui::QtVersionInfo())
    , m_debuggingHelperUi(new Internal::Ui::DebuggingHelper())
    , m_invalidVersionIcon(":/projectexplorer/images/compile_error.png")
    , m_warningVersionIcon(":/projectexplorer/images/compile_warning.png")
    , m_configurationWidget(0)
{
    // Initialize m_versions
    foreach (BaseQtVersion *version, versions)
        m_versions.push_back(version->clone());

    QWidget *versionInfoWidget = new QWidget();
    m_versionUi->setupUi(versionInfoWidget);
    m_versionUi->editPathPushButton->setText(tr(Utils::PathChooser::browseButtonLabel));

    QWidget *debuggingHelperDetailsWidget = new QWidget();
    m_debuggingHelperUi->setupUi(debuggingHelperDetailsWidget);

    m_ui->setupUi(this);

    m_ui->versionInfoWidget->setWidget(versionInfoWidget);
    m_ui->versionInfoWidget->setState(Utils::DetailsWidget::NoSummary);

    m_ui->debuggingHelperWidget->setWidget(debuggingHelperDetailsWidget);

    // setup parent items for auto-detected and manual versions
    m_ui->qtdirList->header()->setResizeMode(QHeaderView::ResizeToContents);
    m_ui->qtdirList->header()->setStretchLastSection(false);
    m_ui->qtdirList->setTextElideMode(Qt::ElideNone);
    QTreeWidgetItem *autoItem = new QTreeWidgetItem(m_ui->qtdirList);
    m_ui->qtdirList->installEventFilter(this);
    autoItem->setText(0, tr("Auto-detected"));
    autoItem->setFirstColumnSpanned(true);
    autoItem->setFlags(Qt::ItemIsEnabled);
    QTreeWidgetItem *manualItem = new QTreeWidgetItem(m_ui->qtdirList);
    manualItem->setText(0, tr("Manual"));
    manualItem->setFirstColumnSpanned(true);
    manualItem->setFlags(Qt::ItemIsEnabled);

    for (int i = 0; i < m_versions.count(); ++i) {
        BaseQtVersion *version = m_versions.at(i);
        QTreeWidgetItem *item = new QTreeWidgetItem(version->isAutodetected()? autoItem : manualItem);
        item->setText(0, version->displayName());
        item->setText(1, QDir::toNativeSeparators(version->qmakeCommand()));
        item->setData(0, VersionIdRole, version->uniqueId());
        const ValidityInfo info = validInformation(version);
        item->setIcon(0, info.icon);
    }
    m_ui->qtdirList->expandAll();

    connect(m_versionUi->nameEdit, SIGNAL(textEdited(const QString &)),
            this, SLOT(updateCurrentQtName()));

    connect(m_versionUi->editPathPushButton, SIGNAL(clicked()),
            this, SLOT(editPath()));

    connect(m_ui->addButton, SIGNAL(clicked()),
            this, SLOT(addQtDir()));
    connect(m_ui->delButton, SIGNAL(clicked()),
            this, SLOT(removeQtDir()));

    connect(m_ui->qtdirList, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this, SLOT(versionChanged(QTreeWidgetItem *, QTreeWidgetItem *)));

    connect(m_debuggingHelperUi->rebuildButton, SIGNAL(clicked()),
            this, SLOT(buildDebuggingHelper()));
    connect(m_debuggingHelperUi->gdbHelperBuildButton, SIGNAL(clicked()),
            this, SLOT(buildGdbHelper()));
    connect(m_debuggingHelperUi->qmlDumpBuildButton, SIGNAL(clicked()),
            this, SLOT(buildQmlDump()));
    connect(m_debuggingHelperUi->qmlDebuggingLibBuildButton, SIGNAL(clicked()),
            this, SLOT(buildQmlDebuggingLibrary()));
    connect(m_debuggingHelperUi->qmlObserverBuildButton, SIGNAL(clicked()),
            this, SLOT(buildQmlObserver()));

    connect(m_debuggingHelperUi->showLogButton, SIGNAL(clicked()),
            this, SLOT(slotShowDebuggingBuildLog()));

    connect(m_ui->cleanUpButton, SIGNAL(clicked()), this, SLOT(cleanUpQtVersions()));
    userChangedCurrentVersion();
    updateCleanUpButton();

    connect(QtVersionManager::instance(), SIGNAL(dumpUpdatedFor(QString)),
            this, SLOT(qtVersionsDumpUpdated(QString)));

    connect(ProjectExplorer::ToolChainManager::instance(), SIGNAL(toolChainsChanged()),
            this, SLOT(toolChainsUpdated()));
}

bool QtOptionsPageWidget::eventFilter(QObject *o, QEvent *e)
{
    // Set the items tooltip, which may cause costly initialization
    // of QtVersion and must be up-to-date
    if (o != m_ui->qtdirList || e->type() != QEvent::ToolTip)
        return false;
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
    const QPoint treePos = helpEvent->pos() - QPoint(0, m_ui->qtdirList->header()->height());
    QTreeWidgetItem *item = m_ui->qtdirList->itemAt(treePos);
    if (!item)
        return false;
    const int index = indexForTreeItem(item);
    if (index == -1)
        return false;
    const QString tooltip = m_versions.at(index)->toHtml(true);
    QToolTip::showText(helpEvent->globalPos(), tooltip, m_ui->qtdirList);
    helpEvent->accept();
    return true;
}

int QtOptionsPageWidget::currentIndex() const
{
    if (QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem())
        return indexForTreeItem(currentItem);
    return -1;
}

BaseQtVersion *QtOptionsPageWidget::currentVersion() const
{
    const int currentItemIndex = currentIndex();
    if (currentItemIndex >= 0 && currentItemIndex < m_versions.size())
        return m_versions.at(currentItemIndex);
    return 0;
}

static inline int findVersionById(const QList<BaseQtVersion *> &l, int id)
{
    const int size = l.size();
    for (int i = 0; i < size; i++)
        if (l.at(i)->uniqueId() == id)
            return i;
    return -1;
}

// Update with results of terminated helper build
void QtOptionsPageWidget::debuggingHelperBuildFinished(int qtVersionId, const QString &output, DebuggingHelperBuildTask::Tools tools)
{
    const int index = findVersionById(m_versions, qtVersionId);
    if (index == -1)
        return; // Oops, somebody managed to delete the version

    BaseQtVersion *version = m_versions.at(index);

    // Update item view
    QTreeWidgetItem *item = treeItemForIndex(index);
    QTC_ASSERT(item, return);
    DebuggingHelperBuildTask::Tools buildFlags
            = item->data(0, BuildRunningRole).value<DebuggingHelperBuildTask::Tools>();
    buildFlags &= ~tools;
    item->setData(0, BuildRunningRole,  QVariant::fromValue(buildFlags));
    item->setData(0, BuildLogRole, output);

    bool success = true;
    if (tools & DebuggingHelperBuildTask::GdbDebugging)
        success &= version->hasGdbDebuggingHelper();
    if (tools & DebuggingHelperBuildTask::QmlDebugging)
        success &= version->hasQmlDebuggingLibrary();
    if (tools & DebuggingHelperBuildTask::QmlDump)
        success &= version->hasQmlDump();
    if (tools & DebuggingHelperBuildTask::QmlObserver)
        success &= version->hasQmlObserver();

    // Update bottom control if the selection is still the same
    if (index == currentIndex()) {
        updateDebuggingHelperUi();
    }

    if (!success)
        showDebuggingBuildLog(item);
}

void QtOptionsPageWidget::cleanUpQtVersions()
{
    QStringList toRemove;
    foreach (const BaseQtVersion *v, m_versions) {
        if (!v->isValid() && !v->isAutodetected())
            toRemove.append(v->displayName());
    }

    if (toRemove.isEmpty())
        return;

    if (QMessageBox::warning(0, tr("Remove invalid Qt Versions"),
                             tr("Do you want to remove all invalid Qt Versions?<br>"
                                "<ul><li>%1</li></ul><br>"
                                "will be removed.").arg(toRemove.join(QLatin1String("</li><li>"))),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;

    for (int i = m_versions.count() - 1; i >= 0; --i) {
        if (!m_versions.at(i)->isValid()) {
            QTreeWidgetItem *item = treeItemForIndex(i);
            delete item;

            delete m_versions.at(i);
            m_versions.removeAt(i);
        }
    }
    updateCleanUpButton();
}

void QtOptionsPageWidget::toolChainsUpdated()
{
    for (int i = 0; i < m_versions.count(); ++i) {
        QTreeWidgetItem *item = treeItemForIndex(i);
        if (item == m_ui->qtdirList->currentItem())
            updateDescriptionLabel();
        else {
            const ValidityInfo info = validInformation(m_versions.at(i));
            item->setIcon(0, info.icon);
        }
    }
}

void QtOptionsPageWidget::qtVersionsDumpUpdated(const QString &qmakeCommand)
{
    foreach (BaseQtVersion *version, m_versions) {
        if (version->qmakeCommand() == qmakeCommand)
            version->recheckDumper();
    }
    if (currentVersion()
            && currentVersion()->qmakeCommand() == qmakeCommand) {
        updateWidgets();
        updateDescriptionLabel();
        updateDebuggingHelperUi();
    }
}

QtOptionsPageWidget::ValidityInfo QtOptionsPageWidget::validInformation(const BaseQtVersion *version)
{
    ValidityInfo info;
    info.icon = m_validVersionIcon;

    if (!version)
        return info;

    if (!version->isValid()) {
        info.icon = m_invalidVersionIcon;
        info.message = version->invalidReason();
        return info;
    }

    // Do we have tool chain issues?
    QStringList missingToolChains;
    int abiCount = 0;
    foreach (const ProjectExplorer::Abi &a, version->qtAbis()) {
        // Ignore symbian emulator since we do not support it.
        if (a.osFlavor() == ProjectExplorer::Abi::SymbianEmulatorFlavor)
            continue;
        if (ProjectExplorer::ToolChainManager::instance()->findToolChains(a).isEmpty())
            missingToolChains.append(a.toString());
        ++abiCount;
    }

    bool useable = true;
    if (missingToolChains.isEmpty()) {
        // No:
        info.message = tr("Qt version %1 for %2").arg(version->qtVersionString(), version->description());
    } else if (missingToolChains.count() == abiCount) {
        // Yes, this Qt version can't be used at all!
        info.message = tr("No tool chain can produce code for this Qt version. Please define one or more tool chains.");
        info.icon = m_invalidVersionIcon;
        useable = false;
    } else {
        // Yes, some ABIs are unsupported
        info.message = tr("Not all possible target environments can be supported due to missing tool chains.");
        info.toolTip = tr("The following ABIs are currently not supported:<ul><li>%1</li></ul>")
                       .arg(missingToolChains.join(QLatin1String("</li><li>")));
        info.icon = m_warningVersionIcon;
    }

    if (useable) {
        QString warning = version->warningReason();
        if (!warning.isEmpty()) {
            if (!info.message.isEmpty())
                info.message.append('\n');
            info.message += warning;
            info.icon = m_warningVersionIcon;
        }
    }

    return info;
}

void QtOptionsPageWidget::buildDebuggingHelper(DebuggingHelperBuildTask::Tools tools)
{
    const int index = currentIndex();
    if (index < 0)
        return;

    QTreeWidgetItem *item = treeItemForIndex(index);
    QTC_ASSERT(item, return);

    DebuggingHelperBuildTask::Tools buildFlags
            = item->data(0, BuildRunningRole).value<DebuggingHelperBuildTask::Tools>();
    buildFlags |= tools;
    item->setData(0, BuildRunningRole, QVariant::fromValue(buildFlags));

    BaseQtVersion *version = m_versions.at(index);
    if (!version)
        return;

    updateDebuggingHelperUi();

    // Run a debugging helper build task in the background.
    DebuggingHelperBuildTask *buildTask = new DebuggingHelperBuildTask(version, tools);
    // Don't open General Messages pane with errors
    buildTask->showOutputOnError(false);
    connect(buildTask, SIGNAL(finished(int,QString,DebuggingHelperBuildTask::Tools)),
            this, SLOT(debuggingHelperBuildFinished(int,QString,DebuggingHelperBuildTask::Tools)),
            Qt::QueuedConnection);
    QFuture<void> task = QtConcurrent::run(&DebuggingHelperBuildTask::run, buildTask);
    const QString taskName = tr("Building helpers");

    Core::ICore::instance()->progressManager()->addTask(task, taskName,
                                                        QLatin1String("Qt4ProjectManager::BuildHelpers"));
}
void QtOptionsPageWidget::buildGdbHelper()
{
    buildDebuggingHelper(DebuggingHelperBuildTask::GdbDebugging);
}

void QtOptionsPageWidget::buildQmlDump()
{
    buildDebuggingHelper(DebuggingHelperBuildTask::QmlDump);
}

void QtOptionsPageWidget::buildQmlDebuggingLibrary()
{
    buildDebuggingHelper(DebuggingHelperBuildTask::QmlDebugging);
}

void QtOptionsPageWidget::buildQmlObserver()
{
    DebuggingHelperBuildTask::Tools qmlDbgTools =
            DebuggingHelperBuildTask::QmlObserver;
    qmlDbgTools |= DebuggingHelperBuildTask::QmlDebugging;
    buildDebuggingHelper(qmlDbgTools);
}

// Non-modal dialog
class BuildLogDialog : public QDialog {
public:
    explicit BuildLogDialog(QWidget *parent = 0);
    void setText(const QString &text);

private:
    Ui_ShowBuildLog m_ui;
};

BuildLogDialog::BuildLogDialog(QWidget *parent) : QDialog(parent)
{
    m_ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
}

void BuildLogDialog::setText(const QString &text)
{
    m_ui.log->setPlainText(text); // Show and scroll to bottom
    m_ui.log->moveCursor(QTextCursor::End);
    m_ui.log->ensureCursorVisible();
}

void QtOptionsPageWidget::slotShowDebuggingBuildLog()
{
    if (const QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem())
        showDebuggingBuildLog(currentItem);
}

void QtOptionsPageWidget::showDebuggingBuildLog(const QTreeWidgetItem *currentItem)
{
    const int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    BuildLogDialog *dialog = new BuildLogDialog(this->window());
    dialog->setWindowTitle(tr("Debugging Helper Build Log for '%1'").arg(currentItem->text(0)));
    dialog->setText(currentItem->data(0, BuildLogRole).toString());
    dialog->show();
}

QtOptionsPageWidget::~QtOptionsPageWidget()
{
    delete m_ui;
    delete m_versionUi;
    delete m_debuggingHelperUi;
    delete m_configurationWidget;
    qDeleteAll(m_versions);
}

static QString filterForQmakeFileDialog()
{
    QString filter("qmake (");
    foreach (const QString &s, Utils::BuildableHelperLibrary::possibleQMakeCommands()) {
#ifdef Q_WS_MAC
        // work around QTBUG-7739 that prohibits filters that don't start with *
        filter += QLatin1Char('*');
#endif
        filter += s + QLatin1Char(' ');
    }
    filter += QLatin1Char(')');
    return filter;
}

void QtOptionsPageWidget::addQtDir()
{
    QString qtVersion = QFileDialog::getOpenFileName(this,
                                                     tr("Select a qmake executable"),
                                                     QString(), filterForQmakeFileDialog());
    if (qtVersion.isNull())
        return;
    if (QtVersionManager::instance()->qtVersionForQMakeBinary(qtVersion)) {
        // Already exist
    }

    BaseQtVersion *version = QtVersionFactory::createQtVersionFromQMakePath(qtVersion);
    if (version) {
        m_versions.append(version);

        QTreeWidgetItem *item = new QTreeWidgetItem(m_ui->qtdirList->topLevelItem(1));
        item->setText(0, version->displayName());
        item->setText(1, QDir::toNativeSeparators(version->qmakeCommand()));
        item->setData(0, VersionIdRole, version->uniqueId());
        item->setIcon(0, version->isValid()? m_validVersionIcon : m_invalidVersionIcon);
        m_ui->qtdirList->setCurrentItem(item); // should update the rest of the ui
        m_versionUi->nameEdit->setFocus();
        m_versionUi->nameEdit->selectAll();
    }
    updateCleanUpButton();
}

void QtOptionsPageWidget::removeQtDir()
{
    QTreeWidgetItem *item = m_ui->qtdirList->currentItem();
    int index = indexForTreeItem(item);
    if (index < 0)
        return;

    delete item;

    BaseQtVersion *version = m_versions.at(index);
    m_versions.removeAt(index);
    delete version;
    updateCleanUpButton();
}

void QtOptionsPageWidget::editPath()
{
    BaseQtVersion *current = currentVersion();
    QString dir = QFileInfo(currentVersion()->qmakeCommand()).absolutePath();
    QString qtVersion = QFileDialog::getOpenFileName(this,
                                                     tr("Select a qmake executable"),
                                                     dir, filterForQmakeFileDialog());
    if (qtVersion.isNull())
        return;
    BaseQtVersion *version = QtVersionFactory::createQtVersionFromQMakePath(qtVersion);
    // Same type? then replace!
    if (current->type() != version->type()) {
        // not the same type, error out
        QMessageBox::critical(this, tr("Qt versions incompatible"),
                              tr("The qt version selected must be for the same target."),
                              QMessageBox::Ok);
        delete version;
        return;
    }
    // same type, replace
    version->setId(current->uniqueId());
    if (current->displayName() != current->defaultDisplayName(current->qtVersionString(), current->qmakeCommand()))
        version->setDisplayName(current->displayName());
    m_versions.replace(m_versions.indexOf(current), version);
    delete current;

    // Update ui
    userChangedCurrentVersion();
    QTreeWidgetItem *item = m_ui->qtdirList->currentItem();
    item->setText(0, version->displayName());
    item->setText(1, QDir::toNativeSeparators(version->qmakeCommand()));
    item->setData(0, VersionIdRole, version->uniqueId());
    item->setIcon(0, version->isValid()? m_validVersionIcon : m_invalidVersionIcon);
}

void QtOptionsPageWidget::updateDebuggingHelperUi()
{
    BaseQtVersion *version = currentVersion();
    const QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();

    if (!version || !version->isValid()) {
        m_ui->debuggingHelperWidget->setVisible(false);
    } else {
        const DebuggingHelperBuildTask::Tools availableTools = DebuggingHelperBuildTask::availableTools(version);
        const bool canBuildGdbHelper = availableTools & DebuggingHelperBuildTask::GdbDebugging;
        const bool canBuildQmlDumper = availableTools & DebuggingHelperBuildTask::QmlDump;
        const bool canBuildQmlDebuggingLib = availableTools & DebuggingHelperBuildTask::QmlDebugging;
        const bool canBuildQmlObserver = availableTools & DebuggingHelperBuildTask::QmlObserver;

        const bool hasGdbHelper = !version->gdbDebuggingHelperLibrary().isEmpty();
        const bool hasQmlDumper = version->hasQmlDump();
        const bool hasQmlDebuggingLib = version->hasQmlDebuggingLibrary();
        const bool needsQmlDebuggingLib = version->needsQmlDebuggingLibrary();
        const bool hasQmlObserver = !version->qmlObserverTool().isEmpty();

        bool isBuildingGdbHelper = false;
        bool isBuildingQmlDumper = false;
        bool isBuildingQmlDebuggingLib = false;
        bool isBuildingQmlObserver = false;

        if (currentItem) {
            DebuggingHelperBuildTask::Tools buildingTools
                    = currentItem->data(0, BuildRunningRole).value<DebuggingHelperBuildTask::Tools>();
            isBuildingGdbHelper = buildingTools & DebuggingHelperBuildTask::GdbDebugging;
            isBuildingQmlDumper = buildingTools & DebuggingHelperBuildTask::QmlDump;
            isBuildingQmlDebuggingLib = buildingTools & DebuggingHelperBuildTask::QmlDebugging;
            isBuildingQmlObserver = buildingTools & DebuggingHelperBuildTask::QmlObserver;
        }

        // get names of tools from labels
        QStringList helperNames;
        if (hasGdbHelper)
            helperNames << m_debuggingHelperUi->gdbHelperLabel->text().remove(':');
        if (hasQmlDumper)
            helperNames << m_debuggingHelperUi->qmlDumpLabel->text().remove(':');
        if (hasQmlDebuggingLib)
            helperNames << m_debuggingHelperUi->qmlDebuggingLibLabel->text().remove(':');
        if (hasQmlObserver)
            helperNames << m_debuggingHelperUi->qmlObserverLabel->text().remove(':');

        QString status;
        if (helperNames.isEmpty()) {
            status = tr("Helpers: None available");
        } else {
            //: %1 is list of tool names.
            status = tr("Helpers: %1.").arg(helperNames.join(QLatin1String(", ")));
        }

        m_ui->debuggingHelperWidget->setSummaryText(status);

        QString gdbHelperText;
        Qt::TextInteractionFlags gdbHelperTextFlags = Qt::NoTextInteraction;
        if (hasGdbHelper) {
            gdbHelperText = QDir::toNativeSeparators(version->gdbDebuggingHelperLibrary());
            gdbHelperTextFlags = Qt::TextSelectableByMouse;
        } else {
            if (canBuildGdbHelper) {
                gdbHelperText =  tr("<i>Not yet built.</i>");
            } else {
                gdbHelperText =  tr("<i>Not needed.</i>");
            }
        }
        m_debuggingHelperUi->gdbHelperStatus->setText(gdbHelperText);
        m_debuggingHelperUi->gdbHelperStatus->setTextInteractionFlags(gdbHelperTextFlags);
        m_debuggingHelperUi->gdbHelperBuildButton->setEnabled(canBuildGdbHelper && !isBuildingGdbHelper);

        QString qmlDumpStatusText, qmlDumpStatusToolTip;
        Qt::TextInteractionFlags qmlDumpStatusTextFlags = Qt::NoTextInteraction;
        if (hasQmlDumper) {
            qmlDumpStatusText = QDir::toNativeSeparators(version->qmlDumpTool(false));
            const QString debugQmlDumpPath = QDir::toNativeSeparators(version->qmlDumpTool(true));
            if (qmlDumpStatusText != debugQmlDumpPath) {
                if (!qmlDumpStatusText.isEmpty()
                        && !debugQmlDumpPath.isEmpty())
                    qmlDumpStatusText += QLatin1String("\n");
                qmlDumpStatusText += debugQmlDumpPath;
            }
            qmlDumpStatusTextFlags = Qt::TextSelectableByMouse;
        } else {
            if (canBuildQmlDumper) {
                qmlDumpStatusText = tr("<i>Not yet built.</i>");
            } else {
                qmlDumpStatusText = tr("<i>Cannot be compiled.</i>");
                QmlDumpTool::canBuild(version, &qmlDumpStatusToolTip);
            }
        }
        m_debuggingHelperUi->qmlDumpStatus->setText(qmlDumpStatusText);
        m_debuggingHelperUi->qmlDumpStatus->setTextInteractionFlags(qmlDumpStatusTextFlags);
        m_debuggingHelperUi->qmlDumpStatus->setToolTip(qmlDumpStatusToolTip);
        m_debuggingHelperUi->qmlDumpBuildButton->setEnabled(canBuildQmlDumper & !isBuildingQmlDumper);

        QString qmlDebuggingLibStatusText, qmlDebuggingLibToolTip;
        Qt::TextInteractionFlags qmlDebuggingLibStatusTextFlags = Qt::NoTextInteraction;
        if (hasQmlDebuggingLib) {
            qmlDebuggingLibStatusText = QDir::toNativeSeparators(
                        version->qmlDebuggingHelperLibrary(false));
            const QString debugPath = QDir::toNativeSeparators(
                        version->qmlDebuggingHelperLibrary(true));

            if (qmlDebuggingLibStatusText != debugPath) {
                if (!qmlDebuggingLibStatusText.isEmpty()
                        && !debugPath.isEmpty()) {
                    qmlDebuggingLibStatusText += QLatin1Char('\n');
                }
                qmlDebuggingLibStatusText += debugPath;
            }
            qmlDebuggingLibStatusTextFlags = Qt::TextSelectableByMouse;
        } else {
            if (!needsQmlDebuggingLib) {
                qmlDebuggingLibStatusText = tr("<i>Not needed.</i>");
            } else if (canBuildQmlDebuggingLib) {
                qmlDebuggingLibStatusText = tr("<i>Not yet built.</i>");
            } else {
                qmlDebuggingLibStatusText = tr("<i>Cannot be compiled.</i>");
                QmlDebuggingLibrary::canBuild(version, &qmlDebuggingLibToolTip);
            }
        }
        m_debuggingHelperUi->qmlDebuggingLibStatus->setText(qmlDebuggingLibStatusText);
        m_debuggingHelperUi->qmlDebuggingLibStatus->setTextInteractionFlags(qmlDebuggingLibStatusTextFlags);
        m_debuggingHelperUi->qmlDebuggingLibStatus->setToolTip(qmlDebuggingLibToolTip);
        m_debuggingHelperUi->qmlDebuggingLibBuildButton->setEnabled(needsQmlDebuggingLib
                                                                    && canBuildQmlDebuggingLib
                                                                    && !isBuildingQmlDebuggingLib);

        QString qmlObserverStatusText, qmlObserverToolTip;
        Qt::TextInteractionFlags qmlObserverStatusTextFlags = Qt::NoTextInteraction;
        if (hasQmlObserver) {
            qmlObserverStatusText = QDir::toNativeSeparators(version->qmlObserverTool());
            qmlObserverStatusTextFlags = Qt::TextSelectableByMouse;
        }  else {
            if (!needsQmlDebuggingLib) {
                qmlObserverStatusText = tr("<i>Not needed.</i>");
            } else if (canBuildQmlObserver) {
                qmlObserverStatusText = tr("<i>Not yet built.</i>");
            } else {
                qmlObserverStatusText = tr("<i>Cannot be compiled.</i>");
                QmlObserverTool::canBuild(version, &qmlObserverToolTip);
            }
        }
        m_debuggingHelperUi->qmlObserverStatus->setText(qmlObserverStatusText);
        m_debuggingHelperUi->qmlObserverStatus->setTextInteractionFlags(qmlObserverStatusTextFlags);
        m_debuggingHelperUi->qmlObserverStatus->setToolTip(qmlObserverToolTip);
        m_debuggingHelperUi->qmlObserverBuildButton->setEnabled(canBuildQmlObserver
                                                                & !isBuildingQmlObserver);

        const bool hasLog = currentItem && !currentItem->data(0, BuildLogRole).toString().isEmpty();
        m_debuggingHelperUi->showLogButton->setEnabled(hasLog);

        m_debuggingHelperUi->rebuildButton->setEnabled((!isBuildingGdbHelper
                                                        && !isBuildingQmlDumper
                                                        && !isBuildingQmlDebuggingLib
                                                        && !isBuildingQmlObserver)
                                                       && (canBuildGdbHelper
                                                           || canBuildQmlDumper
                                                           || (canBuildQmlDebuggingLib && needsQmlDebuggingLib)
                                                           || canBuildQmlObserver));

        m_ui->debuggingHelperWidget->setVisible(true);
    }
}

// To be called if a qt version was removed or added
void QtOptionsPageWidget::updateCleanUpButton()
{
    bool hasInvalidVersion = false;
    for (int i = 0; i < m_versions.count(); ++i) {
        if (!m_versions.at(i)->isValid()) {
            hasInvalidVersion = true;
            break;
        }
    }
    m_ui->cleanUpButton->setEnabled(hasInvalidVersion);
}

void QtOptionsPageWidget::userChangedCurrentVersion()
{
    updateWidgets();
    updateDescriptionLabel();
    updateDebuggingHelperUi();
}

void QtOptionsPageWidget::qtVersionChanged()
{
    updateDescriptionLabel();
    updateDebuggingHelperUi();
}

void QtOptionsPageWidget::updateDescriptionLabel()
{
    QTreeWidgetItem *item = m_ui->qtdirList->currentItem();
    const ValidityInfo info = validInformation(currentVersion());
    m_versionUi->errorLabel->setText(info.message);
    m_versionUi->errorLabel->setToolTip(info.toolTip);
    if (item)
        item->setIcon(0, info.icon);
}

int QtOptionsPageWidget::indexForTreeItem(const QTreeWidgetItem *item) const
{
    if (!item || !item->parent())
        return -1;
    const int uniqueId = item->data(0, VersionIdRole).toInt();
    for (int index = 0; index < m_versions.size(); ++index) {
        if (m_versions.at(index)->uniqueId() == uniqueId)
            return index;
    }
    return -1;
}

QTreeWidgetItem *QtOptionsPageWidget::treeItemForIndex(int index) const
{
    const int uniqueId = m_versions.at(index)->uniqueId();
    for (int i = 0; i < m_ui->qtdirList->topLevelItemCount(); ++i) {
        QTreeWidgetItem *toplevelItem = m_ui->qtdirList->topLevelItem(i);
        for (int j = 0; j < toplevelItem->childCount(); ++j) {
            QTreeWidgetItem *item = toplevelItem->child(j);
            if (item->data(0, VersionIdRole).toInt() == uniqueId) {
                return item;
            }
        }
    }
    return 0;
}

void QtOptionsPageWidget::versionChanged(QTreeWidgetItem *newItem, QTreeWidgetItem *old)
{
    Q_UNUSED(newItem)
    if (old)
        fixQtVersionName(indexForTreeItem(old));
    userChangedCurrentVersion();
}

void QtOptionsPageWidget::updateWidgets()
{
    delete m_configurationWidget;
    m_configurationWidget = 0;
    BaseQtVersion *version = currentVersion();
    if (version) {
        m_versionUi->nameEdit->setText(version->displayName());
        m_versionUi->qmakePath->setText(QDir::toNativeSeparators(version->qmakeCommand()));
        m_configurationWidget = version->createConfigurationWidget();
        if (m_configurationWidget) {
            m_versionUi->formLayout->addRow(m_configurationWidget);
            m_configurationWidget->setEnabled(!version->isAutodetected());
            connect(m_configurationWidget, SIGNAL(changed()),
                    this, SLOT(qtVersionChanged()));
        }
    } else {
        m_versionUi->nameEdit->clear();
        m_versionUi->qmakePath->setText(QString()); // clear()
    }

    const bool enabled = version != 0;
    const bool isAutodetected = enabled && version->isAutodetected();
    m_ui->delButton->setEnabled(enabled && !isAutodetected);
    m_versionUi->nameEdit->setEnabled(enabled && !isAutodetected);
    m_versionUi->editPathPushButton->setEnabled(enabled && !isAutodetected);
}

void QtOptionsPageWidget::updateCurrentQtName()
{
    QTreeWidgetItem *currentItem = m_ui->qtdirList->currentItem();
    Q_ASSERT(currentItem);
    int currentItemIndex = indexForTreeItem(currentItem);
    if (currentItemIndex < 0)
        return;
    m_versions[currentItemIndex]->setDisplayName(m_versionUi->nameEdit->text());
    currentItem->setText(0, m_versions[currentItemIndex]->displayName());
    updateDescriptionLabel();
}


void QtOptionsPageWidget::finish()
{
    if (QTreeWidgetItem *item = m_ui->qtdirList->currentItem())
        fixQtVersionName(indexForTreeItem(item));
}

/* Checks that the qt version name is unique
 * and otherwise changes the name
 *
 */
void QtOptionsPageWidget::fixQtVersionName(int index)
{
    if (index < 0)
        return;
    int count = m_versions.count();
    QString name = m_versions.at(index)->displayName();
    if (name.isEmpty())
        return;
    for (int i = 0; i < count; ++i) {
        if (i != index) {
            if (m_versions.at(i)->displayName() == m_versions.at(index)->displayName()) {
                // Same name, find new name
                QRegExp regexp("^(.*)\\((\\d)\\)$");
                if (regexp.exactMatch(name)) {
                    // Already in Name (#) format
                    name = regexp.cap(1);
                    name += QLatin1Char('(');
                    name += QString::number(regexp.cap(2).toInt() + 1);
                    name += QLatin1Char(')');
                } else {
                    name +=  QLatin1String(" (2)");
                }
                // set new name
                m_versions[index]->setDisplayName(name);
                treeItemForIndex(index)->setText(0, name);

                // Now check again...
                fixQtVersionName(index);
            }
        }
    }
}

QList<BaseQtVersion *> QtOptionsPageWidget::versions() const
{
    QList<BaseQtVersion *> result;
    for (int i = 0; i < m_versions.count(); ++i)
        result.append(m_versions.at(i)->clone());
    return result;
}

QString QtOptionsPageWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream ts(&rc);
    ts << sep << m_versionUi->versionNameLabel->text()
       << sep << m_versionUi->pathLabel->text()
       << sep << m_debuggingHelperUi->gdbHelperLabel->text()
       << sep << m_debuggingHelperUi->qmlDumpLabel->text()
       << sep << m_debuggingHelperUi->qmlObserverLabel->text();

    // Symbian specific, could be factored out to the factory
    // checking m_configurationWidget is not enough, we want them to be a keyword
    // regardless of which qt versions configuration widget is currently active
    ts << sep << tr("S60 SDK:")
       << sep << tr("SBS v2 directory:");


    rc.remove(QLatin1Char('&'));
    return rc;
}
