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
    connect(ui->deployQtLibs, SIGNAL(toggled(bool)), m_step, SLOT(setDeployQtLibs(bool)));
    connect(ui->forceDeploy, SIGNAL(toggled(bool)), m_step, SLOT(setForceDeploy(bool)));
}

AndroidDeployStepWidget::~AndroidDeployStepWidget()
{
    delete ui;
}

void AndroidDeployStepWidget::init()
{
    ui->deployQtLibs->setChecked(m_step->deployQtLibs());
    ui->forceDeploy->setChecked(m_step->forceDeploy());
}

QString AndroidDeployStepWidget::displayName() const
{
    return tr("<b>Deploy configurations</b>");
}

QString AndroidDeployStepWidget::summaryText() const
{

    return displayName();
}


} // namespace Internal
} // namespace Qt4ProjectManager
