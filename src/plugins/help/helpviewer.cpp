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

#include "helpviewer.h"
#include "helpconstants.h"
#include "localhelpmanager.h"

#include <coreplugin/icore.h>

#include <utils/fileutils.h>

#include <QtCore/QFileInfo>
#include <QtCore/QStringBuilder>
#include <QtCore/QDir>
#include <QtCore/QUrl>

#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>
#include <QtGui/QMainWindow>
#include <QtGui/QMouseEvent>

#include <QtHelp/QHelpEngine>

using namespace Help::Internal;

const QString HelpViewer::NsNokia = QLatin1String("qthelp://com.nokia.");
const QString HelpViewer::NsTrolltech = QLatin1String("qthelp://com.trolltech.");

const QString HelpViewer::AboutBlankPage =
    QCoreApplication::translate("HelpViewer", "<title>about:blank</title>");

const QString HelpViewer::PageNotFoundMessage =
    QCoreApplication::translate("HelpViewer", "<html><head><meta http-equiv=\""
    "content-type\" content=\"text/html; charset=UTF-8\"><title>Error 404...</title>"
    "</head><body><div align=\"center\"><br><br><h1>The page could not be found</h1>"
    "<br><h3>'%1'</h3></div></body>");

struct ExtensionMap {
    const char *extension;
    const char *mimeType;
} extensionMap[] = {
    { ".bmp", "image/bmp" },
    { ".css", "text/css" },
    { ".gif", "image/gif" },
    { ".html", "text/html" },
    { ".htm", "text/html" },
    { ".ico", "image/x-icon" },
    { ".jpeg", "image/jpeg" },
    { ".jpg", "image/jpeg" },
    { ".js", "application/x-javascript" },
    { ".mng", "video/x-mng" },
    { ".pbm", "image/x-portable-bitmap" },
    { ".pgm", "image/x-portable-graymap" },
    { ".pdf", "application/pdf" },
    { ".png", "image/png" },
    { ".ppm", "image/x-portable-pixmap" },
    { ".rss", "application/rss+xml" },
    { ".svg", "image/svg+xml" },
    { ".svgz", "image/svg+xml" },
    { ".text", "text/plain" },
    { ".tif", "image/tiff" },
    { ".tiff", "image/tiff" },
    { ".txt", "text/plain" },
    { ".xbm", "image/x-xbitmap" },
    { ".xml", "text/xml" },
    { ".xpm", "image/x-xpm" },
    { ".xsl", "text/xsl" },
    { ".xhtml", "application/xhtml+xml" },
    { ".wml", "text/vnd.wap.wml" },
    { ".wmlc", "application/vnd.wap.wmlc" },
    { "about:blank", 0 },
    { 0, 0 }
};

bool HelpViewer::isLocalUrl(const QUrl &url)
{
    return url.scheme() == QLatin1String("qthelp");
}

bool HelpViewer::canOpenPage(const QString &url)
{
    return !mimeFromUrl(url).isEmpty();
}

QString HelpViewer::mimeFromUrl(const QUrl &url)
{
    const QString &path = url.path();
    const int index = path.lastIndexOf(QLatin1Char('.'));
    const QByteArray &ext = path.mid(index).toUtf8().toLower();

    const ExtensionMap *e = extensionMap;
    while (e->extension) {
        if (ext == e->extension)
            return QLatin1String(e->mimeType);
        ++e;
    }
    return QLatin1String("");
}

bool HelpViewer::launchWithExternalApp(const QUrl &url)
{
    if (isLocalUrl(url)) {
        const QHelpEngineCore &helpEngine = LocalHelpManager::helpEngine();
        const QUrl &resolvedUrl = helpEngine.findFile(url);
        if (!resolvedUrl.isValid())
            return false;

        const QString& path = resolvedUrl.path();
        if (!canOpenPage(path)) {
            Utils::TempFileSaver saver(QDir::tempPath()
                + QLatin1String("/qtchelp_XXXXXX.") + QFileInfo(path).completeSuffix());
            saver.setAutoRemove(false);
            if (!saver.hasError())
                saver.write(helpEngine.fileData(resolvedUrl));
            if (saver.finalize(Core::ICore::instance()->mainWindow()))
                return QDesktopServices::openUrl(QUrl(saver.fileName()));
        }
    }
    return false;
}

void HelpViewer::home()
{
    const QHelpEngineCore &engine = LocalHelpManager::helpEngine();
    QString homepage = engine.customValue(QLatin1String("HomePage"),
        QLatin1String("")).toString();

    if (homepage.isEmpty()) {
        homepage = engine.customValue(QLatin1String("DefaultHomePage"),
            Help::Constants::AboutBlank).toString();
    }

    setSource(homepage);
}

void HelpViewer::slotLoadStarted()
{
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
}

void HelpViewer::slotLoadFinished(bool ok)
{
    Q_UNUSED(ok)
    emit sourceChanged(source());
    qApp->restoreOverrideCursor();
}

bool HelpViewer::handleForwardBackwardMouseButtons(QMouseEvent *event)
{
    if (event->button() == Qt::XButton1) {
        backward();
        return true;
    }
    if (event->button() == Qt::XButton2) {
        forward();
        return true;
    }

    return false;
}
