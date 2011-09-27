/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef MAEMORUNCONFIGURATIONWIDGET_H
#define MAEMORUNCONFIGURATIONWIDGET_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QModelIndex;
class QTableView;
class QToolButton;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace RemoteLinux {
class RemoteLinuxRunConfigurationWidget;
}

namespace Madde {
namespace Internal {
class MaemoRunConfiguration;

class MaemoRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MaemoRunConfigurationWidget(MaemoRunConfiguration *runConfiguration,
        QWidget *parent = 0);

private slots:
    void addMount();
    void removeMount();
    void changeLocalMountDir(const QModelIndex &index);
    void enableOrDisableRemoveMountSpecButton();
    void handleRemoteMountsChanged();
    void updateMountWarning();
    void runConfigurationEnabledChange(bool enabled);

private:
    void addMountWidgets(QVBoxLayout *mainLayout);

    QWidget *m_subWidget;
    QLabel *m_mountWarningLabel;
    QTableView *m_mountView;
    QToolButton *m_removeMountButton;
    Utils::DetailsWidget *m_mountDetailsContainer;
    RemoteLinux::RemoteLinuxRunConfigurationWidget *m_remoteLinuxRunConfigWidget;
    MaemoRunConfiguration *m_runConfiguration;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMORUNCONFIGURATIONWIDGET_H
