/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QT4PROJECTCONFIGWIDGET_H
#define QT4PROJECTCONFIGWIDGET_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
class QAbstractButton;
QT_END_NAMESPACE

namespace Utils {
    class DetailsWidget;
}

namespace Qt4ProjectManager {
class Qt4Target;
class Qt4BuildConfiguration;
class Qt4Target;

namespace Internal {
namespace Ui {
class Qt4ProjectConfigWidget;
}

class Qt4ProjectConfigWidget : public ProjectExplorer::BuildConfigWidget
{
    Q_OBJECT
public:
    explicit Qt4ProjectConfigWidget(Qt4Target *target);
    ~Qt4ProjectConfigWidget();

    QString displayName() const;
    void init(ProjectExplorer::BuildConfiguration *bc);

private slots:
    // User changes in our widgets
    void shadowBuildClicked(bool checked);
    void onBeforeBeforeShadowBuildDirBrowsed();
    void shadowBuildEdited();
    void qtVersionSelected(const QString &);
    void toolChainSelected(int index);
    void manageQtVersions();
    void importLabelClicked();

    // Changes triggered from creator
    void qtVersionsChanged();
    void qtVersionChanged();
    void buildDirectoryChanged();
    void toolChainTypeChanged();
    void updateImportLabel();
    void environmentChanged();
private:
    void updateDetails();
    void updateToolChainCombo();
    void updateShadowBuildUi();

    Ui::Qt4ProjectConfigWidget *m_ui;
    QAbstractButton *m_browseButton;
    Qt4BuildConfiguration *m_buildConfiguration;
    Utils::DetailsWidget *m_detailsContainer;
    bool m_ignoreChange;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4PROJECTCONFIGWIDGET_H
