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

#ifndef PARAMETERACTION_H
#define PARAMETERACTION_H

#include "utils_global.h"

#include <QtGui/QAction>

namespace Utils {

class QTCREATOR_UTILS_EXPORT ParameterAction : public QAction
{
    Q_ENUMS(EnablingMode)
    Q_PROPERTY(QString emptyText READ emptyText WRITE setEmptyText)
    Q_PROPERTY(QString parameterText READ parameterText WRITE setParameterText)
    Q_PROPERTY(EnablingMode enablingMode READ enablingMode WRITE setEnablingMode)
    Q_OBJECT
public:
    enum EnablingMode { AlwaysEnabled, EnabledWithParameter };

    explicit ParameterAction(const QString &emptyText,
                             const QString &parameterText,
                             EnablingMode em = AlwaysEnabled,
                             QObject *parent = 0);

    QString emptyText() const;
    void setEmptyText(const QString &);

    QString parameterText() const;
    void setParameterText(const QString &);

    EnablingMode enablingMode() const;
    void setEnablingMode(EnablingMode m);

public slots:
    void setParameter(const QString &);

private:
    QString m_emptyText;
    QString m_parameterText;
    EnablingMode m_enablingMode;
};

} // namespace Utils

#endif // PARAMETERACTION_H
