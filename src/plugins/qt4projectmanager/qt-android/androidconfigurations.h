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

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
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
    QString GdbLocation;
    QString GdbserverLocation;
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
    QStringList sdkTargets();
    QStringList ndkToolchainVersions();
    QString adbToolPath();
    QString androidToolPath();
    QString antToolPath();
    QString emulatorToolPath();
    QString gccPath();
    QString gdbServerPath();
    QString gdbPath();
    QString stripPath();
    QString readelfPath();
    QString getDeployDeviceSerialNumber(int apiLevel=-1);
    bool createAVD();
    bool createAVD(const QString & target, const QString & name, int sdcardSize );
    bool removeAVD(const QString & name);
    QVector<AndroidDevice> connectedDevices(int apiLevel=-1);
    QVector<AndroidDevice> androidVirtualDevices();
    QString startAVD(int apiLevel, const QString & name = QString());
    QString bestMatch(const QString & targetAPI);
signals:
    void updated();

private:
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
} // namespace Qt4ProjectManager

#endif // ANDROIDDEVICECONFIGURATIONS_H
