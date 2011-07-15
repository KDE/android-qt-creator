/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef QT4ANDROIDTARGETFACTORY_H
#define QT4ANDROIDTARGETFACTORY_H

#include "qt4projectmanager/qt4basetargetfactory.h"
#include "qt4projectmanager/qt4target.h"

namespace Android {
namespace Internal {

class AndroidTargetFactory : public Qt4ProjectManager::Qt4BaseTargetFactory
{
    Q_OBJECT
public:
    AndroidTargetFactory(QObject *parent = 0);
    ~AndroidTargetFactory();

    QStringList supportedTargetIds(ProjectExplorer::Project *parent) const;
    QString displayNameForId(const QString &id) const;
    QIcon iconForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    Qt4ProjectManager::Qt4BaseTarget *restore(ProjectExplorer::Project *parent, const QVariantMap &map);
    QString defaultShadowBuildDirectory(const QString &projectLocation, const QString &id);

    bool supportsTargetId(const QString &id) const;
    virtual QSet<QString> targetFeatures(const QString &id) const;

    Qt4ProjectManager::Qt4BaseTarget *create(ProjectExplorer::Project *parent, const QString &id);
    Qt4ProjectManager::Qt4BaseTarget *create(ProjectExplorer::Project *parent, const QString &id, const QList<Qt4ProjectManager::BuildConfigurationInfo> &infos);

    bool isMobileTarget(const QString &id);
    bool supportsShadowBuilds(const QString &id);
};

} // namespace Internal
} // namespace Android

#endif // QT4ANDROIDTARGETFACTORY_H
