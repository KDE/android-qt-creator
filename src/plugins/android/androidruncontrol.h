/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ANDROIDRUNCONTROL_H
#define ANDROIDRUNCONTROL_H

#include <projectexplorer/runconfiguration.h>

#include <QString>

namespace Android {
namespace Internal {
class AndroidRunConfiguration;
class AndroidRunner;

class AndroidRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    explicit AndroidRunControl(AndroidRunConfiguration *runConfig);
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
