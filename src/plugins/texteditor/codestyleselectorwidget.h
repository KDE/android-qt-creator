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

#ifndef CODESTYLESELECTORWIDGET_H
#define CODESTYLESELECTORWIDGET_H

#include "texteditor_global.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QComboBox;
class QLabel;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

namespace TextEditor {

namespace Internal { namespace Ui { class CodeStyleSelectorWidget; } }

class ICodeStylePreferences;
class ICodeStylePreferencesFactory;

class TEXTEDITOR_EXPORT CodeStyleSelectorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CodeStyleSelectorWidget(ICodeStylePreferencesFactory *factory, QWidget *parent = 0);
    ~CodeStyleSelectorWidget();

    void setCodeStyle(TextEditor::ICodeStylePreferences *codeStyle);
    QString searchKeywords() const;

private slots:
    void slotComboBoxActivated(int index);
    void slotCurrentDelegateChanged(TextEditor::ICodeStylePreferences *delegate);
    void slotCopyClicked();
    void slotEditClicked();
    void slotRemoveClicked();
    void slotImportClicked();
    void slotExportClicked();
    void slotCodeStyleAdded(ICodeStylePreferences*);
    void slotCodeStyleRemoved(ICodeStylePreferences*);
    void slotUpdateName();

private:
    void updateName(ICodeStylePreferences *codeStyle);
    ICodeStylePreferencesFactory *m_factory;
    ICodeStylePreferences *m_codeStyle;

    QString displayName(ICodeStylePreferences *codeStyle) const;

    Internal::Ui::CodeStyleSelectorWidget *m_ui;

    bool m_ignoreGuiSignals;
};

} // namespace TextEditor

#endif // CODESTYLESELECTORWIDGET_H
