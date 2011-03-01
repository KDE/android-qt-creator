/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDRUNFACTORIES_H
#define ANDROIDRUNFACTORIES_H

#include <projectexplorer/runconfiguration.h>

namespace ProjectExplorer {
    class RunConfiguration;
    class RunControl;
    class RunConfigWidget;
    class Target;
}
using ProjectExplorer::IRunConfigurationFactory;
using ProjectExplorer::IRunControlFactory;
using ProjectExplorer::RunConfiguration;
using ProjectExplorer::RunControl;
using ProjectExplorer::RunConfigWidget;
using ProjectExplorer::Target;

namespace Qt4ProjectManager {
    namespace Internal {

class AndroidRunConfigurationFactory : public IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit AndroidRunConfigurationFactory(QObject *parent = 0);
    ~AndroidRunConfigurationFactory();

    QString displayNameForId(const QString &id) const;
    QStringList availableCreationIds(Target *parent) const;

    bool canCreate(Target *parent, const QString &id) const;
    RunConfiguration *create(Target *parent, const QString &id);

    bool canRestore(Target *parent, const QVariantMap &map) const;
    RunConfiguration *restore(Target *parent, const QVariantMap &map);

    bool canClone(Target *parent, RunConfiguration *source) const;
    RunConfiguration *clone(Target *parent, RunConfiguration *source);
};

class AndroidRunControlFactory : public IRunControlFactory
{
    Q_OBJECT
public:
    explicit AndroidRunControlFactory(QObject *parent = 0);
    ~AndroidRunControlFactory();

    QString displayName() const;
    RunConfigWidget *createConfigurationWidget(RunConfiguration *runConfiguration);

    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    RunControl *create(RunConfiguration *runConfiguration, const QString &mode);
};

    } // namespace Internal
} // namespace Qt4ProjectManager

#endif  // ANDROIDRUNFACTORIES_H
