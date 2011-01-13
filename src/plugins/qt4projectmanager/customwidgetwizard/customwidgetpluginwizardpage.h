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

#ifndef CUSTOMWIDGETPLUGINWIZARDPAGE_H
#define CUSTOMWIDGETPLUGINWIZARDPAGE_H

#include "filenamingparameters.h"

#include <QtGui/QWizardPage>
#include <QtCore/QSharedPointer>

namespace Qt4ProjectManager {
namespace Internal {

struct PluginOptions;
class CustomWidgetWidgetsWizardPage;

namespace Ui {
    class CustomWidgetPluginWizardPage;
}

class CustomWidgetPluginWizardPage : public QWizardPage {
    Q_OBJECT
public:
    explicit CustomWidgetPluginWizardPage(QWidget *parent = 0);
    virtual ~CustomWidgetPluginWizardPage();

    void init(const CustomWidgetWidgetsWizardPage *widgetsPage);

    virtual bool isComplete() const;

    FileNamingParameters fileNamingParameters() const { return m_fileNamingParameters; }
    void setFileNamingParameters(const FileNamingParameters &fnp) {m_fileNamingParameters = fnp; }

    // Fills the plugin fields, excluding widget list.
    QSharedPointer<PluginOptions> basicPluginOptions() const;

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_collectionClassEdit_textChanged();
    void on_collectionHeaderEdit_textChanged();
    void slotCheckCompleteness();

private:
    inline QString collectionClassName() const;
    inline QString pluginName() const;
    void setCollectionEnabled(bool enColl);

    Ui::CustomWidgetPluginWizardPage *m_ui;
    FileNamingParameters m_fileNamingParameters;
    int m_classCount;
    bool m_complete;
};

} // namespace Internal
} // namespace Qt4ProjectManager
#endif // CUSTOMWIDGETPLUGINWIZARDPAGE_H
