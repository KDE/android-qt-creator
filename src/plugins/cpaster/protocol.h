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

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QWidget;
QT_END_NAMESPACE

namespace Core {
class IOptionsPage;
}

namespace CodePaster {

class Protocol : public QObject
{
    Q_OBJECT

public:
    enum ContentType {
        Text, C, JavaScript, Diff, Xml
    };

    enum Capabilities  {
        ListCapability = 0x1,
        PostCommentCapability = 0x2,
        PostDescriptionCapability = 0x4
    };

    virtual ~Protocol();

    virtual QString name() const = 0;

    virtual unsigned capabilities() const = 0;
    virtual bool hasSettings() const;
    virtual Core::IOptionsPage *settingsPage() const;

    virtual bool checkConfiguration(QString *errorMessage = 0);
    virtual void fetch(const QString &id) = 0;
    virtual void list();
    virtual void paste(const QString &text,
                       ContentType ct = Text,
                       const QString &username = QString(),
                       const QString &comment = QString(),
                       const QString &description = QString()) = 0;

    // Convenience to determine content type from mime type
    static ContentType contentType(const QString &mimeType);

    // Show a configuration error and point user to settings.
    // Return true when settings changed.
    static bool showConfigurationError(const Protocol *p,
                                       const QString &message,
                                       QWidget *parent = 0,
                                       bool showConfig = true);
    // Ensure configuration is correct
    static bool ensureConfiguration(Protocol *p,
                                    QWidget *parent = 0);

signals:
    void pasteDone(const QString &link);
    void fetchDone(const QString &titleDescription,
                   const QString &content,
                   bool error);
    void listDone(const QString &name, const QStringList &result);

protected:
    Protocol();
    static QString textFromHtml(QString data);
    static QString fixNewLines(QString in);
};

/* Proxy for NetworkAccessManager that can be shared with
 * delayed initialization and conveniences
 * for HTTP-requests. */

class NetworkAccessManagerProxy
{
    Q_DISABLE_COPY(NetworkAccessManagerProxy)

public:
    NetworkAccessManagerProxy();
    ~NetworkAccessManagerProxy();

    QNetworkReply *httpGet(const QString &url);
    QNetworkReply *httpPost(const QString &link, const QByteArray &data);
    QNetworkAccessManager *networkAccessManager();

private:
    QScopedPointer<QNetworkAccessManager> m_networkAccessManager;
};

/* Network-based protocol: Provides access with delayed
 * initialization to a QNetworkAccessManager and conveniences
 * for HTTP-requests. */

class NetworkProtocol : public Protocol
{
    Q_OBJECT

public:
    virtual ~NetworkProtocol();

protected:
    typedef QSharedPointer<NetworkAccessManagerProxy> NetworkAccessManagerProxyPtr;

    explicit NetworkProtocol(const NetworkAccessManagerProxyPtr &nw);

    inline QNetworkReply *httpGet(const QString &url)
    { return m_networkAccessManager->httpGet(url); }

    inline QNetworkReply *httpPost(const QString &link, const QByteArray &data)
    { return m_networkAccessManager->httpPost(link, data); }

    inline QNetworkAccessManager *networkAccessManager()
    { return m_networkAccessManager->networkAccessManager(); }

    // Check connectivity of host, displaying a message box.
    bool httpStatus(QString url, QString *errorMessage);

private:
    const NetworkAccessManagerProxyPtr m_networkAccessManager;
};

} //namespace CodePaster

#endif // PROTOCOL_H
