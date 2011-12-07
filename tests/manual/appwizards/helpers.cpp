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

#include "qtquickapp.h"
#include "html5app.h"
#include <QtCore>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

static bool writeFile(const QByteArray &data, const QString &targetFile)
{
    const QFileInfo fileInfo(targetFile);
    QDir().mkpath(fileInfo.absolutePath());
    QFile file(fileInfo.absoluteFilePath());
    file.open(QIODevice::WriteOnly);
    Q_ASSERT(file.isOpen());
    return file.write(data) != -1;
}

bool QtQuickApp::generateFiles(QString *errorMessage) const
{
    return     writeFile(generateFile(QtQuickAppGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::AppProFile, errorMessage), path(AppPro))
            && (m_mainQmlMode == ModeImport ? true : writeFile(generateFile(QtQuickAppGeneratedFileInfo::MainQmlFile, errorMessage), path(MainQml)))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::SymbianSvgIconFile, errorMessage), path(SymbianSvgIcon))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::PngIcon64File, errorMessage), path(PngIcon64))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::PngIcon80File, errorMessage), path(PngIcon80))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::DesktopFremantleFile, errorMessage), path(DesktopFremantle))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::DesktopHarmattanFile, errorMessage), path(DesktopHarmattan));
}

bool Html5App::generateFiles(QString *errorMessage) const
{
    return     writeFile(generateFile(Html5AppGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::AppProFile, errorMessage), path(AppPro))
            && (mainHtmlMode() != ModeGenerate ? true : writeFile(generateFile(Html5AppGeneratedFileInfo::MainHtmlFile, errorMessage), path(MainHtml)))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::SymbianSvgIconFile, errorMessage), path(SymbianSvgIcon))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::PngIcon64File, errorMessage), path(PngIcon64))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::PngIcon80File, errorMessage), path(PngIcon80))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::DesktopFremantleFile, errorMessage), path(DesktopFremantle))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::DesktopHarmattanFile, errorMessage), path(DesktopHarmattan));
}

QString AbstractMobileApp::templatesRoot()
{
    return QLatin1String("../../../share/qtcreator/templates/");
}
