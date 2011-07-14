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

#include "multifeedrssmodel.h"

#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QCoreApplication>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include "networkaccessmanager.h"

#include <QDebug>

namespace Core {

namespace Internal {

QString shortenHtml(QString html)
{
    html.replace(QLatin1String("<a"), QLatin1String("<i"));
    html.replace(QLatin1String("</a"), QLatin1String("</i"));
    uint firstParaEndXhtml = (uint) html.indexOf(QLatin1String("</p>"));
    uint firstParaEndHtml = (uint) html.indexOf(QLatin1String("<p>"), html.indexOf(QLatin1String("<p>"))+1);
    uint firstParaEndBr = (uint) html.indexOf(QLatin1String("<br"));
    uint firstParaEnd = qMin(firstParaEndXhtml, firstParaEndHtml);
    firstParaEnd = qMin(firstParaEnd, firstParaEndBr);
    return html.left(firstParaEnd);
}

class RssReader {
public:
    Internal::RssItem parseItem() {
        RssItem item;
        item.source = requestUrl;
        item.blogIcon = blogIcon;
        item.blogName = blogName;
        while (!streamReader.atEnd()) {
            switch (streamReader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (streamReader.name() == QLatin1String("title"))
                    item.title = streamReader.readElementText();
                else if (streamReader.name() == QLatin1String("link"))
                    item.link = streamReader.readElementText();
                else if (streamReader.name() == QLatin1String("pubDate")) {
                    QString dateStr = streamReader.readElementText();
                    // fixme: honor time zone!
                    dateStr = dateStr.left(dateStr.indexOf('+')-1);
                    item.pubDate = QDateTime::fromString(dateStr, "ddd, dd MMM yyyy HH:mm:ss");
                }
                else if (streamReader.name() == QLatin1String("description"))
                    item.description = shortenHtml(streamReader.readElementText());
                break;
            case QXmlStreamReader::EndElement:
                if (streamReader.name() == QLatin1String("item"))
                    return item;
                break;
            default:
                break;

            }
        }
        return RssItem();
    }

    Internal::RssItemList parse(QNetworkReply *reply) {
        QUrl source = reply->request().url();
        requestUrl = source.toString();
        streamReader.setDevice(reply);
        Internal::RssItemList list;
        while (!streamReader.atEnd()) {
            switch (streamReader.readNext()) {
            case QXmlStreamReader::StartElement:
                if (streamReader.name() == QLatin1String("item"))
                    list.append(parseItem());
                else if (streamReader.name() == QLatin1String("title"))
                    blogName = streamReader.readElementText();
                else if (streamReader.name() == QLatin1String("link")) {
                    if (!streamReader.namespaceUri().isEmpty())
                        break;
                    QString favIconString(streamReader.readElementText());
                    QUrl favIconUrl(favIconString);
                    favIconUrl.setPath(QLatin1String("favicon.ico"));
                    blogIcon = favIconUrl.toString();
                }
                break;
            default:
                break;
            }
        }
        return list;
    }

private:
    QXmlStreamReader streamReader;
    QString requestUrl;
    QString blogIcon;
    QString blogName;
};

} // namespace Internal

MultiFeedRssModel::MultiFeedRssModel(QObject *parent) :
    QAbstractListModel(parent),
    m_networkAccessManager(new NetworkAccessManager),
    m_articleCount(0)
{
    //m_namThread = new QThread;
    //m_networkAccessManager->moveToThread(m_namThread);
    connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply*)),
            SLOT(appendFeedData(QNetworkReply*)), Qt::QueuedConnection);
    //m_namThread->start();
    //qDebug() << "MainThread" << QThread::currentThread();

    QHash<int, QByteArray> roleNames;
    roleNames[TitleRole] = "title";
    roleNames[DescriptionRole] = "description";
    roleNames[PubDateRole] = "pubDate";
    roleNames[LinkRole] = "link";
    roleNames[BlogNameRole] = "blogName";
    roleNames[BlogIconRole] = "blogIcon";
    setRoleNames(roleNames);
}

MultiFeedRssModel::~MultiFeedRssModel()
{
    //m_namThread->exit();
    //delete m_namThread;
}

void MultiFeedRssModel::addFeed(const QString& feed)
{
    QMetaObject::invokeMethod(m_networkAccessManager, "getUrl",
                              Qt::QueuedConnection, Q_ARG(QUrl, feed));
}

bool sortForPubDate(const Internal::RssItem& item1, const Internal::RssItem& item2)
{
    return item1.pubDate > item2.pubDate;
}

void MultiFeedRssModel::appendFeedData(QNetworkReply *reply)
{
    Internal::RssReader reader;
    m_aggregatedFeed.append(reader.parse(reply));
    qSort(m_aggregatedFeed.begin(), m_aggregatedFeed.end(), sortForPubDate);
    setArticleCount(m_aggregatedFeed.size());
    reset();
}

void MultiFeedRssModel::removeFeed(const QString &feed)
{
    QMutableListIterator<Internal::RssItem> it(m_aggregatedFeed);
    while (it.hasNext()) {
        Internal::RssItem item = it.next();
        if (item.source == feed)
            it.remove();
    }
    setArticleCount(m_aggregatedFeed.size());
}

int MultiFeedRssModel::rowCount(const QModelIndex &) const
{
    return m_aggregatedFeed.size();
}

QVariant MultiFeedRssModel::data(const QModelIndex &index, int role) const
{

    Internal::RssItem item = m_aggregatedFeed.at(index.row());

    switch (role) {
    case Qt::DisplayRole: // fall through
    case TitleRole:
        return item.title;
    case DescriptionRole:
        return item.description;
    case PubDateRole:
        return item.pubDate;
    case LinkRole:
        return item.link;
    case BlogNameRole:
        return item.blogName;
    case BlogIconRole:
        return item.blogIcon;
    }

    return QVariant();
}

} // namespace Utils
