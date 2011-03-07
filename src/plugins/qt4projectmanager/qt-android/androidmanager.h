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


namespace Qt4ProjectManager {
    class QtVersion;
namespace Internal {

class Qt4AndroidTargetFactory;
class AndroidDeployStepFactory;
class AndroidPackageCreationFactory;
class AndroidRunControlFactory;
class AndroidRunConfigurationFactory;
class AndroidSettingsPage;
class AndroidToolChainFactory;

class AndroidManager : public QObject
{
    Q_OBJECT

public:
    AndroidManager();
    ~AndroidManager();
    static AndroidManager &instance();

    bool isValidAndroidQtVersion(const Qt4ProjectManager::QtVersion *version) const;

    AndroidSettingsPage *settingsPage() const { return m_settingsPage; }

private:
    static AndroidManager *m_instance;

    AndroidRunControlFactory *m_runControlFactory;
    AndroidRunConfigurationFactory *m_runConfigurationFactory;
    AndroidPackageCreationFactory *m_packageCreationFactory;
    AndroidDeployStepFactory *m_deployStepFactory;
    AndroidSettingsPage *m_settingsPage;
    Qt4AndroidTargetFactory * m_androidTargetFactory;
    AndroidToolChainFactory *m_toolChainFactory;
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDMANAGER_H
