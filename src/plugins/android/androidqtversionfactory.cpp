/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidqtversionfactory.h"
#include "androidqtversion.h"
#include "androidconstants.h"
#include <qtsupport/qtsupportconstants.h>
#include <utils/qtcassert.h>
#include <proparser/profileevaluator.h>

#include <QtCore/QFileInfo>

namespace Android {
namespace Internal {

AndroidQtVersionFactory::AndroidQtVersionFactory(QObject *parent)
    : QtSupport::QtVersionFactory(parent)
{

}

AndroidQtVersionFactory::~AndroidQtVersionFactory()
{

}

bool AndroidQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(Constants::ANDROIDQT);
}

QtSupport::BaseQtVersion *AndroidQtVersionFactory::restore(const QString &type,
    const QVariantMap &data)
{
    QTC_ASSERT(canRestore(type), return 0);
    AndroidQtVersion *v = new AndroidQtVersion;
    v->fromMap(data);
    return v;
}

int AndroidQtVersionFactory::priority() const
{
    return 90;
}

QtSupport::BaseQtVersion *AndroidQtVersionFactory::create(const QString &qmakePath, ProFileEvaluator *evaluator, bool isAutoDetected, const QString &autoDetectionSource)
{
    QFileInfo fi(qmakePath);
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;
    if (!evaluator->values("CONFIG").contains(QLatin1String("android")))
        return 0;
    return new AndroidQtVersion(qmakePath, isAutoDetected, autoDetectionSource);
}

} // Internal
} // Android
