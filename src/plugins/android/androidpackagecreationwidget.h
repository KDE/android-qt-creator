/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDPACKAGECREATIONWIDGET_H
#define ANDROIDPACKAGECREATIONWIDGET_H

#include <projectexplorer/buildstep.h>
#include <QtCore/QAbstractListModel>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class AndroidPackageCreationWidget; }
QT_END_NAMESPACE

namespace Android {
namespace Internal {
class AndroidPackageCreationStep;

class CheckModel: public QAbstractListModel
{
    Q_OBJECT
public:
    CheckModel(QObject * parent = 0 );
    void setAvailableItems(const QStringList &items);
    void setCheckedItems(const QStringList &items);
    const QStringList &checkedItems();
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
    PermissionsModel(QObject *parent = 0 );
    void setPermissions(const QStringList &permissions);
    const QStringList & permissions();
    QModelIndex addPermission(const QString &permission);
    bool updatePermission(QModelIndex index,const QString &permission);
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

public slots:
    void readElfInfo();

private:
    void setEnabledSaveDiscardButtons(bool enabled);
    void setCertificates();

private slots:
    void initGui();
    void updateAndroidProjectInfo();

    void setPackageName();
    void setApplicationName();
    void setTargetSDK(const QString &target);
    void setVersionCode();
    void setVersionName();
    void setTarget(const QString &target);

    void setQtLibs(QModelIndex,QModelIndex);
    void setPrebundledLibs(QModelIndex,QModelIndex);
    void prebundledLibSelected(const QModelIndex &index);
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
    void updateRequiredLibrariesModels();
    void on_signPackageCheckBox_toggled(bool checked);
    void on_KeystoreCreatePushButton_clicked();
    void on_KeystoreLocationPushButton_clicked();
    void on_certificatesAliasComboBox_activated(const QString &alias);
    void on_certificatesAliasComboBox_currentIndexChanged(const QString &alias);

    void on_openPackageLocationCheckBox_toggled(bool checked);

private:
    AndroidPackageCreationStep *const m_step;
    Ui::AndroidPackageCreationWidget *const m_ui;
    CheckModel * m_qtLibsModel;
    CheckModel * m_prebundledLibs;
    PermissionsModel * m_permissionsModel;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDPACKAGECREATIONWIDGET_H
