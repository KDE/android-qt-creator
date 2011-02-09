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

#ifndef GENERICPROJECTWIZARD_H
#define GENERICPROJECTWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <utils/wizard.h>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QDir;
QT_END_NAMESPACE

namespace Utils {

class FileWizardPage;

} // namespace Utils

namespace GenericProjectManager {
namespace Internal {

    class GenericProjectWizardDialog : public Utils::Wizard
{
    Q_OBJECT

public:
    GenericProjectWizardDialog(QWidget *parent = 0);
    virtual ~GenericProjectWizardDialog();

    QString path() const;
    void setPath(const QString &path);

    QString projectName() const;

    Utils::FileWizardPage *m_firstPage;
};

class GenericProjectWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    GenericProjectWizard();
    virtual ~GenericProjectWizard();

    static Core::BaseFileWizardParameters parameters();

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;

    virtual bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l, QString *errorMessage);

    bool isValidDir(const QFileInfo &fileInfo) const;

    void getFileList(const QDir &dir, const QString &projectRoot,
                     const QStringList &suffixes,
                     QStringList *files,
                     QStringList *paths) const;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICPROJECTWIZARD_H
