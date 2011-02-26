/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDMANAGER_H
#define ANDROIDMANAGER_H

#include <QtCore/QObject>

namespace ProjectExplorer {
    class ToolChain;
}
using ProjectExplorer::ToolChain;

namespace Qt4ProjectManager {
    class QtVersion;
namespace Internal {

class Qt4AndroidTargetFactory;
class AndroidDeployStepFactory;
class AndroidPackageCreationFactory;
class AndroidRunControlFactory;
class AndroidRunConfigurationFactory;
class AndroidSettingsPage;
class AndroidQemuManager;

class AndroidManager : public QObject
{
    Q_OBJECT

public:
    AndroidManager();
    ~AndroidManager();
    static AndroidManager &instance();

    bool isValidAndroidQtVersion(const Qt4ProjectManager::QtVersion *version) const;
    ToolChain *androidToolChain() const;

    AndroidSettingsPage *settingsPage() const { return m_settingsPage; }

private:
    static AndroidManager *m_instance;

    AndroidRunControlFactory *m_runControlFactory;
    AndroidRunConfigurationFactory *m_runConfigurationFactory;
    AndroidPackageCreationFactory *m_packageCreationFactory;
    AndroidDeployStepFactory *m_deployStepFactory;
    AndroidSettingsPage *m_settingsPage;
    AndroidQemuManager *m_qemuRuntimeManager;
    Qt4AndroidTargetFactory * m_androidTargetFactory;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDMANAGER_H
