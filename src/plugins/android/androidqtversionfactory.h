/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDQTVERSIONFACTORY_H
#define ANDROIDQTVERSIONFACTORY_H
#include <qtsupport/qtversionfactory.h>

namespace Android {
namespace Internal {

class AndroidQtVersionFactory : public QtSupport::QtVersionFactory
{
public:
    explicit AndroidQtVersionFactory(QObject *parent = 0);
    ~AndroidQtVersionFactory();

    virtual bool canRestore(const QString &type);
    virtual QtSupport::BaseQtVersion *restore(const QString &type, const QVariantMap &data);

    virtual int priority() const;
    virtual QtSupport::BaseQtVersion *create(const Utils::FileName &qmakePath, ProFileEvaluator *evaluator, bool isAutoDetected = false, const QString &autoDetectionSource = QString());
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDQTVERSIONFACTORY_H
