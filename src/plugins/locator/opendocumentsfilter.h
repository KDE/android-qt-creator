/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef OPENDOCUMENTSFILTER_H
#define OPENDOCUMENTSFILTER_H

#include "ilocatorfilter.h"

#include <coreplugin/editormanager/openeditorsmodel.h>

#include <QString>
#include <QList>
#include <QByteArray>
#include <QFutureInterface>

namespace Core {
    class EditorManager;
}

namespace Locator {
namespace Internal {

class OpenDocumentsFilter : public Locator::ILocatorFilter
{
    Q_OBJECT

public:
    explicit OpenDocumentsFilter(Core::EditorManager *editorManager);
    QString displayName() const { return tr("Open Documents"); }
    QString id() const { return QLatin1String("Open documents"); }
    Locator::ILocatorFilter::Priority priority() const { return Locator::ILocatorFilter::Medium; }
    QList<Locator::FilterEntry> matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);

public slots:
    void refreshInternally();

private:
    Core::EditorManager *m_editorManager;

    QList<Core::OpenEditorsModel::Entry> m_editors;
};

} // namespace Internal
} // namespace Locator

#endif // OPENDOCUMENTSFILTER_H
