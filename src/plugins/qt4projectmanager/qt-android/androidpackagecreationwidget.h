/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ANDROIDPACKAGECREATIONWIDGET_H
#define ANDROIDPACKAGECREATIONWIDGET_H

#include <projectexplorer/buildstep.h>
#include <QtCore/QAbstractListModel>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class AndroidPackageCreationWidget; }
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class AndroidPackageCreationStep;

class CheckModel: public QAbstractListModel
{
    Q_OBJECT
public:
    CheckModel(QObject * parent = 0 );
    void setAvailableItems(const QStringList & items);
    void setCheckedItems(const QStringList & items);
    const QStringList & checkedItems();
    QVariant data(const QModelIndex &index, int role) const;
    void swap(int index1, int index2);
    int rowCount(const QModelIndex &parent) const;

protected:
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
private:
    QStringList m_availableItems;
    QStringList m_checkedItems;
};

class PermissionsModel: public QAbstractListModel
{
    Q_OBJECT
public:
    PermissionsModel(QObject * parent = 0 );
    void setPermissions(const QStringList & permissions);
    const QStringList & permissions();
    QModelIndex addPermission(const QString & permission);
    bool updatePermission(QModelIndex index,const QString & permission);
    void removePermission(int index);
    QVariant data(const QModelIndex &index, int role) const;

protected:
    int rowCount(const QModelIndex &parent) const;

private:
    QStringList m_permissions;
};

class AndroidPackageCreationWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    AndroidPackageCreationWidget(AndroidPackageCreationStep *step);

    virtual void init();
    virtual QString summaryText() const;
    virtual QString displayName() const;

private:
    void setEnabledSaveDiscardButtons(bool enabled);

private slots:
    void initGui();
    void updateAndroidProjectInfo();

    void setPackageName();
    void setApplicationName();
    void setTargetSDK(const QString & target);
    void setVersionCode();
    void setVersionName();
    void setTarget(const QString & target);

    void setQtLibs(QModelIndex,QModelIndex);
    void setPrebundledLibs(QModelIndex,QModelIndex);
    void prebundledLibSelected(const QModelIndex & index);
    void prebundledLibMoveUp();
    void prebundledLibMoveDown();

    void setHDPIIcon();
    void setMDPIIcon();
    void setLDPIIcon();

    void permissionActivated(QModelIndex index);
    void addPermission();
    void updatePermission();
    void removePermission();
    void savePermissionsButton();
    void discardPermissionsButton();

    void readElfInfo();

private:
    AndroidPackageCreationStep * const m_step;
    Ui::AndroidPackageCreationWidget * const m_ui;
    CheckModel * m_qtLibsModel;
    CheckModel * m_prebundledLibs;
    PermissionsModel * m_permissionsModel;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDPACKAGECREATIONWIDGET_H
