/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#include "androidruncontrol.h"

#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidrunner.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

using ProjectExplorer::RunConfiguration;
using namespace ProjectExplorer;

AndroidRunControl::AndroidRunControl(RunConfiguration *rc)
    : RunControl(rc, ProjectExplorer::Constants::RUNMODE)
    , m_runner(new AndroidRunner(this, qobject_cast<AndroidRunConfiguration *>(rc), false))
    , m_running(false)
{
}

AndroidRunControl::~AndroidRunControl()
{
    stop();
}

void AndroidRunControl::start()
{
    m_running = true;
    emit started();
    disconnect(m_runner, 0, this, 0);

    connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)),
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteOutput(QByteArray)),
        SLOT(handleRemoteOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteProcessFinished(const QString &)),
        SLOT(handleRemoteProcessFinished(const QString &)));
    appendMessage(tr("Starting remote process ..."), NormalMessageFormat);
    m_runner->start();
}

ProjectExplorer::RunControl::StopResult AndroidRunControl::stop()
{
    m_runner->stop();
    return StoppedSynchronously;
}

void AndroidRunControl::handleRemoteProcessFinished(const QString &error)
{
    appendMessage(error , ErrorMessageFormat);
    disconnect(m_runner, 0, this, 0);
    m_running = false;
    emit finished();
}

void AndroidRunControl::handleRemoteOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), StdOutFormatSameLine);
}

void AndroidRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), StdErrFormatSameLine);
}

bool AndroidRunControl::isRunning() const
{
    return m_running;
}

QString AndroidRunControl::displayName() const
{
    return m_runner->displayName();
}

} // namespace Internal
} // namespace Qt4ProjectManager
