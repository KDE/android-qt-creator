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

#ifndef PROJECTEXPLORER_ABIWIDGET_H
#define PROJECTEXPLORER_ABIWIDGET_H

#include "projectexplorer_export.h"

#include <QtGui/QWidget>

namespace ProjectExplorer {
class Abi;

namespace Internal {
class AbiWidgetPrivate;
} // namespace

// --------------------------------------------------------------------------
// AbiWidget:
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT AbiWidget : public QWidget
{
    Q_OBJECT

public:
    AbiWidget(QWidget *parent = 0);
    ~AbiWidget();

    void setAbis(const QList<Abi> &, const Abi &current);
    Abi currentAbi() const;

signals:
    void abiChanged();

private slots:
    void osChanged();
    void modeChanged();

private:
    void setCustomAbi(const Abi &a);

    Internal::AbiWidgetPrivate *const d;
};

} // namespace ProjectExplorer

#endif // PROJECTEXPLORER_ABIWIDGET_H
