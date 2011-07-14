/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDQTVERSION_H
#define ANDROIDQTVERSION_H
#include <qtsupport/baseqtversion.h>

namespace Qt4ProjectManager{

namespace Internal {

class AndroidQtVersion  : public QtSupport::BaseQtVersion
{
public:
    AndroidQtVersion();
    AndroidQtVersion(const QString &path, bool isAutodetected = false, const QString &autodetectionSource = QString());
    ~AndroidQtVersion ();
    AndroidQtVersion  *clone() const;

    virtual QString type() const;

    virtual bool isValid() const;
    virtual QString invalidReason() const;

    virtual QList<ProjectExplorer::Abi> qtAbis() const;

    virtual bool supportsTargetId(const QString &id) const;
    virtual QSet<QString> supportedTargetIds() const;

    QString description() const;

private:
    mutable bool m_qtAbisUpToDate;
    mutable QList<ProjectExplorer::Abi> m_qtAbis;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDQTVERSION_H
