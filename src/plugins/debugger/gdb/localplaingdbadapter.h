/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_PLAINGDBADAPTER_H
#define DEBUGGER_PLAINGDBADAPTER_H

#include "abstractplaingdbadapter.h"
#include "localgdbprocess.h"
#include <outputcollector.h>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PlainGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class LocalPlainGdbAdapter : public AbstractPlainGdbAdapter
{
    Q_OBJECT

public:
    explicit LocalPlainGdbAdapter(GdbEngine *engine, QObject *parent = 0);

private:
    void startAdapter();
    void setupInferior();
    void runEngine();
    void interruptInferior();
    void shutdownInferior();
    void shutdownAdapter();

    DumperHandling dumperHandling() const;
    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }

    virtual QByteArray execFilePath() const;
    virtual bool infoTargetNecessary() const;
    virtual QByteArray toLocalEncoding(const QString &s) const;
    virtual QString fromLocalEncoding(const QByteArray &b) const;
    void checkForReleaseBuild();

    OutputCollector m_outputCollector;
    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_PLAINGDBADAPTER_H
