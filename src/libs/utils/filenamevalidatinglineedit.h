/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef FILENAMEVALIDATINGLINEEDIT_H
#define FILENAMEVALIDATINGLINEEDIT_H

#include "basevalidatinglineedit.h"

namespace Utils {

/**
 * A control that let's the user choose a file name, based on a QLineEdit. Has
 * some validation logic for embedding into QWizardPage.
 */
class QTCREATOR_UTILS_EXPORT FileNameValidatingLineEdit : public BaseValidatingLineEdit
{
    Q_OBJECT
    Q_DISABLE_COPY(FileNameValidatingLineEdit)
    Q_PROPERTY(bool allowDirectories READ allowDirectories WRITE setAllowDirectories)
public:
    explicit FileNameValidatingLineEdit(QWidget *parent = 0);

    static bool validateFileName(const QString &name,
                                 bool allowDirectories = false,
                                 QString *errorMessage = 0);

    /**
     * Sets whether entering directories is allowed. This will enable the user
     * to enter slashes in the filename. Default is off.
     */
    bool allowDirectories() const;
    void setAllowDirectories(bool v);

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const;

private:
    bool m_allowDirectories;
    void *m_unused;
};

} // namespace Utils

#endif // FILENAMEVALIDATINGLINEEDIT_H
