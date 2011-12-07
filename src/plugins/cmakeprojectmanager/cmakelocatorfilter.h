/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 Kläralvdalens Datakonsult AB, a KDAB Group company.
**
** Contact: Kläralvdalens Datakonsult AB (info@kdab.com)
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

#ifndef CMAKEPROJECTMANAGER_INTERNAL_CMAKEFILTER_H
#define CMAKEPROJECTMANAGER_INTERNAL_CMAKEFILTER_H

#include <locator/ilocatorfilter.h>

#include <QtGui/QIcon>

namespace CMakeProjectManager {
namespace Internal {

class CMakeLocatorFilter : public Locator::ILocatorFilter
{
    Q_OBJECT

public:
    CMakeLocatorFilter();
    ~CMakeLocatorFilter();

    QString displayName() const { return tr("Build CMake target"); }
    QString id() const { return QLatin1String("Build CMake target"); }
    Priority priority() const { return Medium; }

    QList<Locator::FilterEntry> matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);

private slots:
    void slotProjectListUpdated();
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEPROJECTMANAGER_INTERNAL_CMAKEFILTER_H
