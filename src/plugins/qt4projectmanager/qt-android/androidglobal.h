/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDGLOBAL_H
#define ANDROIDGLOBAL_H

#include <utils/environment.h>

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QList>

#define ASSERT_STATE_GENERIC(State, expected, actual)                         \
    AndroidGlobal::assertState<State>(expected, actual, Q_FUNC_INFO)

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class AndroidGlobal
{
public:

    template<class T> static T *buildStep(const ProjectExplorer::DeployConfiguration *dc)
    {
        ProjectExplorer::BuildStepList *bsl = dc->stepList();
        if (!bsl)
            return 0;
        const QList<ProjectExplorer::BuildStep *> &buildSteps = bsl->steps();
        for (int i = buildSteps.count() - 1; i >= 0; --i) {
            if (T * const step = qobject_cast<T *>(buildSteps.at(i)))
                return step;
        }
        return 0;
    }

    template<typename State> static void assertState(State expected,
        State actual, const char *func)
    {
        assertState(QList<State>() << expected, actual, func);
    }

    template<typename State> static void assertState(const QList<State> &expected,
        State actual, const char *func)
    {
        if (!expected.contains(actual)) {
            qWarning("Warning: Unexpected state %d in function %s.",
                actual, func);
        }
    }
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDGLOBAL_H
