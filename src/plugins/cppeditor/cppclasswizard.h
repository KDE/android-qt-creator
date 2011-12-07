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

#ifndef CPPCLASSWIZARD_H
#define CPPCLASSWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <utils/wizard.h>

#include <QtCore/QStringList>
#include <QtGui/QWizardPage>

namespace Utils {

class NewClassWidget;

} // namespace Utils

namespace CppEditor {
namespace Internal {

class ClassNamePage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ClassNamePage(QWidget *parent = 0);

    bool isComplete() const { return m_isValid; }
    Utils::NewClassWidget *newClassWidget() const { return m_newClassWidget; }

private slots:
    void slotValidChanged();

private:
    void initParameters();

    Utils::NewClassWidget *m_newClassWidget;
    bool m_isValid;
};


struct CppClassWizardParameters
{
    QString className;
    QString headerFile;
    QString sourceFile;
    QString baseClass;
    QString path;
    int classType;
};

class CppClassWizardDialog : public Utils::Wizard
{
    Q_OBJECT

public:
    explicit CppClassWizardDialog(QWidget *parent = 0);

    void setPath(const QString &path);
    CppClassWizardParameters parameters() const;

private:
    ClassNamePage *m_classNamePage;
};


class CppClassWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    explicit CppClassWizard(const Core::BaseFileWizardParameters &parameters,
                            QObject *parent = 0);

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;


    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;
    QString sourceSuffix() const;
    QString headerSuffix() const;

private:
    static bool generateHeaderAndSource(const CppClassWizardParameters &params,
                                        QString *header, QString *source);

};

} // namespace Internal
} // namespace CppEditor

#endif // CPPCLASSWIZARD_H
