/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDSETTINGSWIDGET_H
#define ANDROIDSETTINGSWIDGET_H

#include "androidconfigurations.h"

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtGui/QWidget>
#include <QtCore/QAbstractTableModel>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QModelIndex;

class Ui_AndroidSettingsWidget;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class AVDModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    void setAvdList(QVector<AndroidDevice> list);
    QString avdName(const QModelIndex & index);

protected:
    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

private:
    QVector<AndroidDevice> m_list;
};

class AndroidSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    AndroidSettingsWidget(QWidget *parent);
    ~AndroidSettingsWidget();

    void saveSettings(bool saveNow = false);
    QString searchKeywords() const;

private slots:
    void SDKLocationEditingFinished();
    void NDKLocationEditingFinished();
    void AntLocationEditingFinished();
    void GdbLocationEditingFinished();
    void GdbserverLocationEditingFinished();
    void OpenJDKLocationEditingFinished();
    void browseSDKLocation();
    void browseNDKLocation();
    void browseAntLocation();
    void browseGdbLocation();
    void browseGdbserverLocation();
    void browseOpenJDKLocation();
    void toolchainVersionIndexChanged(QString);
    void addAVD();
    void removeAVD();
    void startAVD();
    void avdActivated(QModelIndex);
    void DataPartitionSizeEditingFinished();

private:
    void initGui();
    bool checkSDK(const QString & location);
    bool checkNDK(const QString & location);
    void fillToolchainVersions();

    Ui_AndroidSettingsWidget *m_ui;
    AndroidConfig m_androidConfig;
    AVDModel m_AVDModel;
    bool m_saveSettingsRequested;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDSETTINGSWIDGET_H
