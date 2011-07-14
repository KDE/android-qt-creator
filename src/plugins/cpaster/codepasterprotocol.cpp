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

#include "codepasterprotocol.h"
#include "codepastersettings.h"
#include "cpasterplugin.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/messageoutputwindow.h>

#include <utils/qtcassert.h>

#include <QtGui/QListWidget>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QDebug>

enum { debug = 0 };

namespace CodePaster {

CodePasterProtocol::CodePasterProtocol(const NetworkAccessManagerProxyPtr &nw) :
    NetworkProtocol(nw),
    m_page(new CodePaster::CodePasterSettingsPage),
    m_pasteReply(0),
    m_fetchReply(0),
    m_listReply(0),
    m_fetchId(-1)
{
}

CodePasterProtocol::~CodePasterProtocol()
{
}

QString CodePasterProtocol::name() const
{
    return QLatin1String("CodePaster");
}

unsigned CodePasterProtocol::capabilities() const
{
    return ListCapability|PostCommentCapability|PostDescriptionCapability;
}

bool CodePasterProtocol::checkConfiguration(QString *errorMessage)
{
    const QString hostName = m_page->hostName();
    if (hostName.isEmpty()) {
        if (errorMessage) {
            *errorMessage =
#ifdef Q_OS_MAC
                       tr("No Server defined in the CodePaster preferences.");
#else
                       tr("No Server defined in the CodePaster options.");
#endif
        }
        return false;
    }
    // Check the host once. Note that it can be modified in the settings page.
    if (m_hostChecked == hostName)
        return true;
    const bool ok = httpStatus(m_page->hostName(), errorMessage);
    if (ok)
        m_hostChecked = hostName;
    return ok;
}

void CodePasterProtocol::fetch(const QString &id)
{
    QTC_ASSERT(!m_fetchReply, return; )

    QString hostName = m_page->hostName();
    const QString httpPrefix = "http://";
    QString link;
    // Did the user enter a complete URL instead of an id?
    if (id.startsWith(httpPrefix)) {
        // Append 'raw' format option
        link = id;
        link += QLatin1String("&format=raw");
        const int idPos = id.lastIndexOf(QLatin1Char('='));
        m_fetchId = idPos != -1 ? id.mid(idPos + 1) : id;
    } else {
        link = httpPrefix;
        link.append(hostName);
        link.append("/?format=raw&id=");
        link.append(id);
        m_fetchId = id;
    }
    m_fetchReply = httpGet(link);
    connect(m_fetchReply, SIGNAL(finished()), this, SLOT(fetchFinished()));
}

void CodePasterProtocol::list()
{
    QTC_ASSERT(!m_listReply, return; )

    QString hostName = m_page->hostName();
    QString link = QLatin1String("http://");
    link += hostName;
    link += QLatin1String("/?command=browse&format=raw");
    m_listReply = httpGet(link);
    connect(m_listReply, SIGNAL(finished()), this, SLOT(listFinished()));
}

void CodePasterProtocol::paste(const QString &text,
                               ContentType /* ct */,
                               const QString &username,
                               const QString &comment,
                               const QString &description)
{
    QTC_ASSERT(!m_pasteReply, return; )
    const QString hostName = m_page->hostName();

    QByteArray data = "command=processcreate&submit=submit&highlight_type=0&description=";
    data += QUrl::toPercentEncoding(description);
    data += "&comment=";
    data += QUrl::toPercentEncoding(comment);
    data += "&code=";
    data += QUrl::toPercentEncoding(fixNewLines(text));
    data += "&poster=";
    data += QUrl::toPercentEncoding(username);

    m_pasteReply = httpPost(QLatin1String("http://") + hostName, data);
    connect(m_pasteReply, SIGNAL(finished()), this, SLOT(pasteFinished()));
}

void CodePasterProtocol::pasteFinished()
{
    if (m_pasteReply->error()) {
        qWarning("Error pasting: %s", qPrintable(m_pasteReply->errorString()));
    } else {
        // Cut out the href-attribute
        QString contents = QString::fromAscii(m_pasteReply->readAll());
        int hrefPos = contents.indexOf(QLatin1String("href=\""));
        if (hrefPos != -1) {
            hrefPos += 6;
            const int endPos = contents.indexOf(QLatin1Char('"'), hrefPos);
            if (endPos != -1)
                emit pasteDone(contents.mid(hrefPos, endPos - hrefPos));
        }
    }
    m_pasteReply->deleteLater();
    m_pasteReply = 0;
}

bool CodePasterProtocol::hasSettings() const
{
    return true;
}

Core::IOptionsPage *CodePasterProtocol::settingsPage() const
{
    return m_page;
}

void CodePasterProtocol::fetchFinished()
{
    QString title;
    QString content;
    bool error = m_fetchReply->error();
    if (error) {
        content = m_fetchReply->errorString();
    } else {
        content = m_fetchReply->readAll();
        if (debug)
            qDebug() << content;
        if (content.contains("<B>No such paste!</B>")) {
            content = tr("No such paste");
            error = true;
        }
        title = QString::fromLatin1("Codepaster: %1").arg(m_fetchId);
    }
    m_fetchReply->deleteLater();
    m_fetchReply = 0;
    emit fetchDone(title, content, error);
}

void CodePasterProtocol::listFinished()
{
    if (m_listReply->error()) {
        Core::ICore::instance()->messageManager()->printToOutputPane(m_listReply->errorString(), true);
    } else {
        const QByteArray data = m_listReply->readAll();
        const QStringList lines = QString::fromAscii(data).split(QLatin1Char('\n'));
        emit listDone(name(), lines);
    }
    m_listReply->deleteLater();
    m_listReply = 0;
}

} // namespace CodePaster
