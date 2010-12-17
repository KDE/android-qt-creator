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

#ifndef FORMWIZARDPAGE_H
#define FORMWIZARDPAGE_H

#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE
class QDesignerNewFormWidgetInterface;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

// A wizard page embedding Qt Designer's QDesignerNewFormWidgetInterface
// widget.

class FormTemplateWizardPage : public QWizardPage
{
    Q_DISABLE_COPY(FormTemplateWizardPage)
    Q_OBJECT
public:
    explicit FormTemplateWizardPage(QWidget * parent = 0);

    virtual bool isComplete () const;
    virtual bool validatePage();

    QString templateContents() const { return  m_templateContents; }

    // Parse UI XML forms to determine:
    // 1) The ui class name from "<class>Designer::Internal::FormClassWizardPage</class>"
    // 2) the base class from: widget class="QWizardPage"...
    static bool getUIXmlData(const QString &uiXml, QString *formBaseClass, QString *uiClassName);
    // Change the class name in a UI XML form
    static QString changeUiClassName(const QString &uiXml, const QString &newUiClassName);
    static QString stripNamespaces(const QString &className);

signals:
    void templateActivated();

private slots:
    void slotCurrentTemplateChanged(bool);

private:
    QString m_templateContents;
    QDesignerNewFormWidgetInterface *m_newFormWidget;
    bool m_templateSelected;
};

} // namespace Internal
} // namespace Designer

#endif // FORMTEMPLATEWIZARDPAGE_H
