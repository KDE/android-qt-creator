/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

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
    void editRulesFile();

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
