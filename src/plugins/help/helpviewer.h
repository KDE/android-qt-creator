/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <find/ifindsupport.h>

#include <QtCore/qglobal.h>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariant>

#include <QtGui/QAction>
#include <QtGui/QFont>

#if defined(QT_NO_WEBKIT)
#include <QtGui/QTextBrowser>
#else
#include <QtWebKit/QWebView>
#endif

namespace Help {
namespace Internal {

#if !defined(QT_NO_WEBKIT)
class HelpViewer : public QWebView
#else
class HelpViewer : public QTextBrowser
#endif
{
    Q_OBJECT
    class HelpViewerPrivate;

public:
    explicit HelpViewer(qreal zoom, QWidget *parent = 0);
    ~HelpViewer();

    QFont viewerFont() const;
    void setViewerFont(const QFont &font);

    void scaleUp();
    void scaleDown();

    void resetScale();
    qreal scale() const;

    QString title() const;
    void setTitle(const QString &title);

    QUrl source() const;
    void setSource(const QUrl &url);

    QString selectedText() const;
    bool isForwardAvailable() const;
    bool isBackwardAvailable() const;

    bool findText(const QString &text, Find::FindFlags flags,
        bool incremental, bool fromSearch, bool *wrapped = 0);

    static const QString NsNokia;
    static const QString NsTrolltech;
    static const QString AboutBlankPage;
    static const QString PageNotFoundMessage;

    static bool isLocalUrl(const QUrl &url);
    static bool canOpenPage(const QString &url);
    static QString mimeFromUrl(const QUrl &url);
    static bool launchWithExternalApp(const QUrl &url);

public slots:
    void copy();
    void home();

    void forward();
    void backward();

signals:
    void titleChanged();
    void printRequested();
    void openFindToolBar();

#if !defined(QT_NO_WEBKIT)
    void sourceChanged(const QUrl &);
    void forwardAvailable(bool enabled);
    void backwardAvailable(bool enabled);
#else
    void loadFinished(bool finished);
#endif

protected:
    void keyPressEvent(QKeyEvent *e);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private slots:
    void actionChanged();
    void slotLoadStarted();
    void slotLoadFinished(bool ok);
#if !defined(QT_NO_WEBKIT)
    void slotNetworkReplyFinished(QNetworkReply *reply);
#endif

private:
    bool eventFilter(QObject *obj, QEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    QVariant loadResource(int type, const QUrl &name);
    bool handleForwardBackwardMouseButtons(QMouseEvent *e);

private:
    HelpViewerPrivate *d;
};

}   // namespace Internal
}   // namespace Help

#endif  // HELPVIEWER_H
