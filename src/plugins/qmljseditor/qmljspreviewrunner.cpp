/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljspreviewrunner.h"

#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>

#include <QtGui/QMessageBox>
#include <QtGui/QApplication>

#include <QtCore/QDebug>

namespace QmlJSEditor {
namespace Internal {

QmlJSPreviewRunner::QmlJSPreviewRunner(QObject *parent) :
    QObject(parent)
{
    // prepend creator/bin dir to search path (only useful for special creator-qml package)
    const QString searchPath = QCoreApplication::applicationDirPath()
                               + Utils::SynchronousProcess::pathSeparator()
                               + QString(qgetenv("PATH"));
    m_qmlViewerDefaultPath = Utils::SynchronousProcess::locateBinary(searchPath, QLatin1String("qmlviewer"));

    Utils::Environment environment = Utils::Environment::systemEnvironment();
    m_applicationLauncher.setEnvironment(environment);
}

bool QmlJSPreviewRunner::isReady() const
{
    return !m_qmlViewerDefaultPath.isEmpty();
}

void QmlJSPreviewRunner::run(const QString &filename)
{
    QString errorMessage;
    if (!filename.isEmpty()) {
        m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui, m_qmlViewerDefaultPath,
                                    Utils::QtcProcess::quoteArg(filename));

    } else {
        errorMessage = "No file specified.";
    }

    if (!errorMessage.isEmpty())
        QMessageBox::warning(0, tr("Failed to preview Qt Quick file"),
                             tr("Could not preview Qt Quick (QML) file. Reason: \n%1").arg(errorMessage));
}


} // namespace Internal
} // namespace QmlJSEditor
