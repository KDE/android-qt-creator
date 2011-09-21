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

#ifndef CODEPASTERPLUGIN_H
#define CODEPASTERPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace CodePaster {
class CustomFetcher;
class CustomPoster;
struct Settings;
class Protocol;

class CodePasterService : public QObject
{
    Q_OBJECT
public:
    explicit CodePasterService(QObject *parent = 0);

public slots:
    void postText(const QString &text, const QString &mimeType);
    void postCurrentEditor();
    void postClipboard();
};

class CodepasterPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    CodepasterPlugin();
    ~CodepasterPlugin();

    virtual bool initialize(const QStringList &arguments, QString *errorMessage);
    virtual void extensionsInitialized();
    virtual ShutdownFlag aboutToShutdown();

    static CodepasterPlugin *instance();

public slots:
    void postEditor();
    void postClipboard();
    void fetch();
    void finishPost(const QString &link);
    void finishFetch(const QString &titleDescription,
                     const QString &content,
                     bool error);

    void post(QString data, const QString &mimeType);
private:

    static CodepasterPlugin *m_instance;
    const QSharedPointer<Settings> m_settings;
    QAction *m_postEditorAction;
    QAction *m_postClipboardAction;
    QAction *m_fetchAction;
    QList<Protocol*> m_protocols;
    QStringList m_fetchedSnippets;
};

} // namespace CodePaster

#endif // CODEPASTERPLUGIN_H
