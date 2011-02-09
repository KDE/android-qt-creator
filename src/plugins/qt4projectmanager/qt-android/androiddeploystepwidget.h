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

private slots:
    void resetAction();
    void setDeployLocalQtLibs();
    void setQASIPackagePath();
    void useLocalQtLibsStateChanged(int);

private:

    virtual void init();
    virtual QString summaryText() const;
    virtual QString displayName() const;

    Ui::AndroidDeployStepWidget *ui;
    AndroidDeployStep * m_step;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDDEPLOYSTEPWIDGET_H
