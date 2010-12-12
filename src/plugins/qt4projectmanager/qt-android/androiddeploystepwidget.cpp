#include "androiddeploystepwidget.h"
#include "ui_androiddeploystepwidget.h"

#include "androiddeploystep.h"
#include "androiddeviceconfiglistmodel.h"
#include "androidrunconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

AndroidDeployStepWidget::AndroidDeployStepWidget(AndroidDeployStep *step) :
    ProjectExplorer::BuildStepConfigWidget(),
    ui(new Ui::AndroidDeployStepWidget),
    m_step(step)
{
    ui->setupUi(this);
//    ui->modelComboBox->setModel(m_step->deployables().data());
//    connect(m_step->deployables().data(), SIGNAL(modelAboutToBeReset()),
//        SLOT(handleModelListToBeReset()));

    // Queued connection because of race condition with combo box's reaction
    // to modelReset().
//    connect(m_step->deployables().data(), SIGNAL(modelReset()),
//        SLOT(handleModelListReset()), Qt::QueuedConnection);

    connect(ui->modelComboBox, SIGNAL(currentIndexChanged(int)),
        SLOT(setModel(int)));
    connect(ui->addDesktopFileButton, SIGNAL(clicked()),
        SLOT(addDesktopFile()));
    handleModelListReset();
}

AndroidDeployStepWidget::~AndroidDeployStepWidget()
{
    delete ui;
}

void AndroidDeployStepWidget::init()
{
    handleDeviceConfigModelChanged();
    connect(m_step->buildConfiguration()->target(),
        SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
        this, SLOT(handleDeviceConfigModelChanged()));
    connect(ui->deviceConfigComboBox, SIGNAL(activated(int)), this,
        SLOT(setCurrentDeviceConfig(int)));
    ui->deployToSysrootCheckBox->setChecked(m_step->isDeployToSysrootEnabled());
    connect(ui->deployToSysrootCheckBox, SIGNAL(toggled(bool)), this,
        SLOT(setDeployToSysroot(bool)));
    handleDeviceConfigModelChanged();
}

void AndroidDeployStepWidget::handleDeviceConfigModelChanged()
{
    const AndroidDeviceConfigListModel * const oldModel
        = qobject_cast<AndroidDeviceConfigListModel *>(ui->deviceConfigComboBox->model());
    if (oldModel)
        disconnect(oldModel, 0, this, 0);
//    AndroidDeviceConfigListModel * const devModel = m_step->deviceConfigModel();
//    ui->deviceConfigComboBox->setModel(devModel);
//    connect(devModel, SIGNAL(currentChanged()), this,
//        SLOT(handleDeviceUpdate()));
//    connect(devModel, SIGNAL(modelReset()), this,
//        SLOT(handleDeviceUpdate()));
    handleDeviceUpdate();
}

void AndroidDeployStepWidget::handleDeviceUpdate()
{
#warning FIXME Android
    ui->deviceConfigComboBox->setCurrentIndex(0);//m_step->deviceConfigModel()->currentIndex());
    emit updateSummary();
}

QString AndroidDeployStepWidget::summaryText() const
{
#warning FIXME Android
    return "";//tr("<b>Deploy to device</b>: %1").arg(m_step->deviceConfig().name);
}

QString AndroidDeployStepWidget::displayName() const
{
    return QString();
}

void AndroidDeployStepWidget::setCurrentDeviceConfig(int index)
{
    m_step->deviceConfigModel()->setCurrentIndex(index);
}

void AndroidDeployStepWidget::setDeployToSysroot(bool doDeploy)
{
    m_step->setDeployToSysrootEnabled(doDeploy);
}

void AndroidDeployStepWidget::handleModelListToBeReset()
{
    ui->tableView->reset(); // Otherwise we'll crash if the user is currently editing.
    ui->tableView->setModel(0);
    ui->addDesktopFileButton->setEnabled(false);
}

void AndroidDeployStepWidget::handleModelListReset()
{
//    QTC_ASSERT(m_step->deployables()->modelCount() == ui->modelComboBox->count(), return);
//    if (m_step->deployables()->modelCount() > 0) {
//        if (ui->modelComboBox->currentIndex() == -1)
//            ui->modelComboBox->setCurrentIndex(0);
//        else
//            setModel(ui->modelComboBox->currentIndex());
//    }
}

void AndroidDeployStepWidget::setModel(int row)
{
//    bool canAddDesktopFile = false;
//    if (row != -1) {
//        AndroidDeployableListModel *const model
//            = m_step->deployables()->modelAt(row);
//        ui->tableView->setModel(model);
//        ui->tableView->resizeRowsToContents();
//        canAddDesktopFile = model->canAddDesktopFile();
//    }
//    ui->addDesktopFileButton->setEnabled(canAddDesktopFile);
}

void AndroidDeployStepWidget::addDesktopFile()
{
    const int modelRow = ui->modelComboBox->currentIndex();
    if (modelRow == -1)
        return;
//    AndroidDeployableListModel *const model
//        = m_step->deployables()->modelAt(modelRow);
//    QString error;
//    if (!model->addDesktopFile(error)) {
//        QMessageBox::warning(this, tr("Could not create desktop file"),
//             tr("Error creating desktop file: %1").arg(error));
//    }
//    ui->addDesktopFileButton->setEnabled(model->canAddDesktopFile());
//    ui->tableView->resizeRowsToContents();
}

} // namespace Internal
} // namespace Qt4ProjectManager
