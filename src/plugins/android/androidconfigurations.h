/*
I BogDan Vatra < bog_dan_ro@yahoo.com >, the copyright holder of this work,
hereby release it into the public domain. This applies worldwide.

In case this is not legally possible, I grant any entity the right to use
this work for any purpose, without any conditions, unless such conditions
are required by law.
*/

#ifndef ANDROIDDEVICECONFIGURATIONS_H
#define ANDROIDDEVICECONFIGURATIONS_H


#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <projectexplorer/abi.h>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

#ifdef Q_OS_LINUX
    const QLatin1String ToolchainHost("linux-x86");
#else
# ifdef Q_OS_DARWIN
    const QLatin1String ToolchainHost("darwin-x86");
# else
#  ifdef Q_OS_WIN32
    const QLatin1String ToolchainHost("windows");
#  else
#  error No Android supported OSs found
#  endif
# endif
#endif

class AndroidConfig
{
public:
    AndroidConfig();
    AndroidConfig(const QSettings &settings);
    void save(QSettings &settings) const;

    QString SDKLocation;
    QString NDKLocation;
    QString NDKToolchainVersion;
    QString AntLocation;
    QString ArmGdbLocation;
    QString ArmGdbserverLocation;
    QString X86GdbLocation;
    QString X86GdbserverLocation;
    QString OpenJDKLocation;
    unsigned PartitionSize;
};

struct AndroidDevice{
    QString serialNumber;
    int sdk;
};

class AndroidConfigurations : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AndroidConfigurations)
public:

    static AndroidConfigurations &instance(QObject *parent = 0);
    AndroidConfig config() const { return m_config; }
    void setConfig(const AndroidConfig &config);
    QStringList sdkTargets(int minApiLevel=0);
    QStringList ndkToolchainVersions();
    QString adbToolPath();
    QString androidToolPath();
    QString antToolPath();
    QString emulatorToolPath();
    QString gccPath(ProjectExplorer::Abi::Architecture architecture);
    QString gdbServerPath(ProjectExplorer::Abi::Architecture architecture);
    QString gdbPath(ProjectExplorer::Abi::Architecture architecture);
    QString openJDKPath();
    QString stripPath(ProjectExplorer::Abi::Architecture architecture);
    QString readelfPath(ProjectExplorer::Abi::Architecture architecture);
    QString getDeployDeviceSerialNumber(int & apiLevel);
    bool createAVD(const QString & target, const QString & name, int sdcardSize );
    bool removeAVD(const QString & name);
    QVector<AndroidDevice> connectedDevices(int apiLevel=-1);
    QVector<AndroidDevice> androidVirtualDevices();
    QString startAVD(int & apiLevel, const QString & name = QString());
    QString bestMatch(const QString & targetAPI);

    static const QLatin1String & toolchainPrefix(ProjectExplorer::Abi::Architecture architecture);
    static const QLatin1String & toolsPrefix(ProjectExplorer::Abi::Architecture architecture);

signals:
    void updated();

public slots:
    bool createAVD(int minApiLevel=0);

private:
    QString toolPath(ProjectExplorer::Abi::Architecture architecture);

    AndroidConfigurations(QObject *parent);
    void load();
    void save();

    int getSDKVersion(const QString & device);
    void updateAvailablePlatforms();
private:
    static AndroidConfigurations *m_instance;
    AndroidConfig m_config;
    QVector<int> m_availablePlatforms;
    friend class AndroidConfig;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDDEVICECONFIGURATIONS_H
