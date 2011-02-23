/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDRUNCONFIGURATIONWIDGET_H
#define ANDROIDRUNCONFIGURATIONWIDGET_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QModelIndex;
class QPushButton;
class QRadioButton;
class QTableView;
class QToolButton;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils {
class EnvironmentItem;
}

namespace ProjectExplorer {
class EnvironmentWidget;
}

namespace Utils { class DetailsWidget; }

namespace Qt4ProjectManager {

class Qt4BuildConfiguration;

namespace Internal {

class AndroidRunConfiguration;

class AndroidRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AndroidRunConfigurationWidget(AndroidRunConfiguration *runConfiguration,
                                         QWidget *parent = 0);

private slots:
    void runConfigurationEnabledChange(bool enabled);
    void argumentsEdited(const QString &args);
    void showSettingsDialog(const QString &link);
    void updateTargetInformation();
    void handleDebuggingTypeChanged(bool useGdb);
    void fetchEnvironment();
    void fetchEnvironmentFinished();
    void fetchEnvironmentError(const QString &error);
    void stopFetchEnvironment();
    void userChangesEdited();
    void baseEnvironmentSelected(int index);
    void baseEnvironmentChanged();
    void systemEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges);
    void handleDebuggingTypeChanged();
    void handleDeploySpecsChanged();
    void handleBuildConfigChanged();

private:
    void addGenericWidgets(QVBoxLayout *mainLayout);
    void addDebuggingWidgets(QVBoxLayout *mainLayout);
    void addMountWidgets(QVBoxLayout *mainLayout);
    void addEnvironmentWidgets(QVBoxLayout *mainLayout);

    QLineEdit *m_argsLineEdit;
    QLabel *m_localExecutableLabel;
    QLabel *m_remoteExecutableLabel;
    QLabel *m_devConfLabel;
    QLabel *m_debuggingLanguagesLabel;
    QRadioButton *m_debugCppOnlyButton;
    QRadioButton *m_debugQmlOnlyButton;
    QRadioButton *m_debugCppAndQmlButton;
    QLabel *m_mountWarningLabel;
    QTableView *m_mountView;
    QToolButton *m_removeMountButton;
    Utils::DetailsWidget *m_mountDetailsContainer;
    Utils::DetailsWidget *m_debugDetailsContainer;
    AndroidRunConfiguration *m_runConfiguration;

    bool m_ignoreChange;
    QPushButton *m_fetchEnv;
    QComboBox *m_baseEnvironmentComboBox;
    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
    Qt4BuildConfiguration *m_lastActiveBuildConfig;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // ANDROIDRUNCONFIGURATIONWIDGET_H
