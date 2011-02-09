#include "androiddeploystepwidget.h"
#include "ui_androiddeploystepwidget.h"

#include "androiddeploystep.h"
#include "androiddeviceconfiglistmodel.h"
#include "androidrunconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>
#include <QtGui/QFileDialog>

namespace Qt4ProjectManager {
namespace Internal {

AndroidDeployStepWidget::AndroidDeployStepWidget(AndroidDeployStep *step) :
    ProjectExplorer::BuildStepConfigWidget(),
    ui(new Ui::AndroidDeployStepWidget),
    m_step(step)
{
    ui->setupUi(this);
    connect(m_step, SIGNAL(resetDelopyAction()), SLOT(resetAction()));
    connect(ui->devicesQtLibs, SIGNAL(clicked()), SLOT(resetAction()));
    connect(ui->deployQtLibs, SIGNAL(clicked()), SLOT(setDeployLocalQtLibs()));
    connect(ui->chooseButton, SIGNAL(clicked()), SLOT(setQASIPackagePath()));
    connect(ui->useLocalQtLibs, SIGNAL(stateChanged(int)), SLOT(useLocalQtLibsStateChanged(int)));
}

AndroidDeployStepWidget::~AndroidDeployStepWidget()
{
    delete ui;
}

void AndroidDeployStepWidget::init()
{
    ui->useLocalQtLibs->setChecked(m_step->useLocalQtLibs());
    switch(m_step->deployAction())
    {
        case AndroidDeployStep::DeployLocal:
            ui->deployQtLibs->setChecked(true);
            break;
        default:
            ui->devicesQtLibs->setChecked(true);
            break;
    }
}

QString AndroidDeployStepWidget::displayName() const
{
    return tr("<b>Deploy configurations</b>");
}

QString AndroidDeployStepWidget::summaryText() const
{

    return displayName();
}

void AndroidDeployStepWidget::resetAction()
{
    ui->devicesQtLibs->setChecked(true);
    m_step->setDeployAction(AndroidDeployStep::NoDeploy);
}

void AndroidDeployStepWidget::setDeployLocalQtLibs()
{
    m_step->setDeployAction(AndroidDeployStep::DeployLocal);
}

void AndroidDeployStepWidget::setQASIPackagePath()
{
    QString packagePath = QFileDialog::getOpenFileName(this, tr("Qt Android smart installer"), QDir::homePath(), tr("Android package (*.apk)"));
    if (packagePath.length())
        m_step->setDeployQASIPackagePath(packagePath);
}

void AndroidDeployStepWidget::useLocalQtLibsStateChanged(int state)
{
    m_step->setUseLocalQtLibs(state == Qt::Checked);
}

} // namespace Internal
} // namespace Qt4ProjectManager
