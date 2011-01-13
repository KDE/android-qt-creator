/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PROJECTINTROPAGE_H
#define PROJECTINTROPAGE_H

#include "utils_global.h"

#include <QtGui/QWizardPage>

namespace Utils {

struct ProjectIntroPagePrivate;

/* Standard wizard page for a single file letting the user choose name
 * and path. Looks similar to FileWizardPage, but provides additional
 * functionality:
 * - Description label at the top for displaying introductory text
 * - It does on the fly validation (connected to changed()) and displays
 *   warnings/errors in a status label at the bottom (the page is complete
 *   when fully validated, validatePage() is thus not implemented).
 *
 * Note: Careful when changing projectintropage.ui. It must have main
 * geometry cleared and QLayout::SetMinimumSize constraint on the main
 * layout, otherwise, QWizard will squeeze it due to its strange expanding
 * hacks. */

class QTCREATOR_UTILS_EXPORT ProjectIntroPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(ProjectIntroPage)
    Q_PROPERTY(QString description READ description WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString projectName READ projectName WRITE setProjectName DESIGNABLE true)
    Q_PROPERTY(bool useAsDefaultPath READ useAsDefaultPath WRITE setUseAsDefaultPath DESIGNABLE true)
public:
    explicit ProjectIntroPage(QWidget *parent = 0);
    virtual ~ProjectIntroPage();

    QString projectName() const;
    QString path() const;
    QString description() const;
    bool useAsDefaultPath() const;

    // Insert an additional control into the form layout for the target.
    void insertControl(int row, QWidget *label, QWidget *control);

    virtual bool isComplete() const;

    // Validate a project directory name entry field
    static bool validateProjectDirectory(const QString &name, QString *errorMessage);

signals:
    void activated();

public slots:
    void setPath(const QString &path);
    void setProjectName(const QString &name);
    void setDescription(const QString &description);
    void setUseAsDefaultPath(bool u);

private slots:
    void slotChanged();
    void slotActivated();

protected:
    virtual void changeEvent(QEvent *e);

private:
    enum StatusLabelMode { Error, Warning, Hint };

    bool validate();
    void displayStatusMessage(StatusLabelMode m, const QString &);
    void hideStatusLabel();

    ProjectIntroPagePrivate *m_d;
};

} // namespace Utils

#endif // PROJECTINTROPAGE_H
