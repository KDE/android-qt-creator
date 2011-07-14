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

#ifndef PROJECTNAMEVALIDATINGLINEEDIT_H
#define PROJECTNAMEVALIDATINGLINEEDIT_H

#include "basevalidatinglineedit.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT ProjectNameValidatingLineEdit : public BaseValidatingLineEdit
{
    Q_OBJECT
    Q_DISABLE_COPY(ProjectNameValidatingLineEdit)

public:
    explicit ProjectNameValidatingLineEdit(QWidget *parent = 0);

    static bool validateProjectName(const QString &name, QString *errorMessage /* = 0*/);

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const;
};

} // namespace Utils

#endif // PROJECTNAMEVALIDATINGLINEEDIT_H
