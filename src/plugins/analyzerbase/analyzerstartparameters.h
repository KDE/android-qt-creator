/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ANALYZERSTARTPARAMETERS_H
#define ANALYZERSTARTPARAMETERS_H

#include "analyzerbase_global.h"
#include "analyzerconstants.h"

#include <QtCore/QMetaType>

#include <coreplugin/id.h>
#include <utils/ssh/sshconnection.h>
#include <utils/environment.h>

namespace Analyzer {

// Note: This is part of the "soft interface" of the analyzer plugin.
// Do not add anything that needs implementation in a .cpp file.

class ANALYZER_EXPORT AnalyzerStartParameters
{
public:
    AnalyzerStartParameters()
        : connParams(Utils::SshConnectionParameters::NoProxy)
    {}

    StartMode startMode;
    Utils::SshConnectionParameters connParams;

    Core::Id toolId;
    QString debuggee;
    QString debuggeeArgs;
    QString analyzerCmdPrefix;
    QString displayName;
    Utils::Environment environment;
    QString workingDirectory;
    QString sysroot;
};

} // namespace Analyzer

Q_DECLARE_METATYPE(Analyzer::AnalyzerStartParameters)

#endif // ANALYZERSTARTPARAMETERS_H
