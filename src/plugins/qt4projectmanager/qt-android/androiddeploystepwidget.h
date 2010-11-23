#ifndef ANDROIDDEPLOYSTEPWIDGET_H
#define ANDROIDDEPLOYSTEPWIDGET_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class AndroidDeployStepWidget;
}
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class AndroidDeployStep;

class AndroidDeployStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    AndroidDeployStepWidget(AndroidDeployStep *step);
    ~AndroidDeployStepWidget();

private:
    Q_SLOT void handleDeviceUpdate();
    Q_SLOT void handleDeviceConfigModelChanged();
    Q_SLOT void setCurrentDeviceConfig(int index);
    Q_SLOT void setDeployToSysroot(bool doDeloy);
    Q_SLOT void setModel(int row);
    Q_SLOT void handleModelListToBeReset();
    Q_SLOT void handleModelListReset();
    Q_SLOT void addDesktopFile();

    virtual void init();
    virtual QString summaryText() const;
    virtual QString displayName() const;

    Ui::AndroidDeployStepWidget *ui;
    AndroidDeployStep * const m_step;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDDEPLOYSTEPWIDGET_H
