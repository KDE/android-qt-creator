/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDRUNCONTROL_H
#define ANDROIDRUNCONTROL_H

#include <projectexplorer/runconfiguration.h>

#include <QtCore/QString>

namespace Android {
namespace Internal {
class AndroidRunConfiguration;
class AndroidRunner;

class AndroidRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    explicit AndroidRunControl(ProjectExplorer::RunConfiguration *runConfig);
    virtual ~AndroidRunControl();

    virtual void start();
    virtual StopResult stop();
    virtual bool isRunning() const;
    virtual QString displayName() const;
    virtual QIcon icon() const;

private slots:
    void handleRemoteProcessFinished(const QString &error);
    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);

private:

    AndroidRunner * const m_runner;
    bool m_running;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDRUNCONTROL_H
