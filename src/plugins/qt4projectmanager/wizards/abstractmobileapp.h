/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef ABSTRACTMOBILEAPP_H
#define ABSTRACTMOBILEAPP_H

#include "../qt4projectmanager_global.h"
#include <QtCore/QFileInfo>
#include <QtCore/QPair>

#ifndef CREATORLESSTEST
#include <coreplugin/basefilewizard.h>
#endif // CREATORLESSTEST

QT_FORWARD_DECLARE_CLASS(QTextStream)

namespace Qt4ProjectManager {

/// \internal
struct
#ifndef CREATORLESSTEST
    QT4PROJECTMANAGER_EXPORT
#endif // CREATORLESSTEST
    AbstractGeneratedFileInfo
{
    enum FileType {
        MainCppFile,
        AppProFile,
        DeploymentPriFile,
        SymbianSvgIconFile,
        MaemoPngIconFile64,
        MaemoPngIconFile80,
        DesktopFileFremantle,
        DesktopFileHarmattan,
        ExtendedFile
    };

    AbstractGeneratedFileInfo();

    int fileType;
    QFileInfo fileInfo;
    int currentVersion; // Current version of the template file in Creator
    int version; // The version in the file header
    quint16 dataChecksum; // The calculated checksum
    quint16 statedChecksum; // The checksum in the file header
};

typedef QPair<QString, QString> DeploymentFolder; // QPair<.source, .target>

/// \internal
class
#ifndef CREATORLESSTEST
    QT4PROJECTMANAGER_EXPORT
#endif // CREATORLESSTEST
    AbstractMobileApp : public QObject
{
    Q_OBJECT

public:
    enum ScreenOrientation {
        ScreenOrientationLockLandscape,
        ScreenOrientationLockPortrait,
        ScreenOrientationAuto,
        ScreenOrientationImplicit // Don't set in application at all (used by Symbian components)
    };

    enum FileType {
        MainCpp,
        MainCppOrigin,
        AppPro,
        AppProOrigin,
        AppProPath,
        DesktopFremantle,
        DesktopHarmattan,
        DesktopOrigin,
        DeploymentPri,
        DeploymentPriOrigin,
        SymbianSvgIcon,
        SymbianSvgIconOrigin,
        MaemoPngIcon64,
        MaemoPngIconOrigin64,
        MaemoPngIcon80,
        MaemoPngIconOrigin80,
        ExtendedFile
    };

    virtual ~AbstractMobileApp();

    void setOrientation(ScreenOrientation orientation);
    ScreenOrientation orientation() const;
    void setProjectName(const QString &name);
    QString projectName() const;
    void setProjectPath(const QString &path);
    void setSymbianSvgIcon(const QString &icon);
    QString symbianSvgIcon() const;
    void setMaemoPngIcon64(const QString &icon);
    QString maemoPngIcon64() const;
    void setMaemoPngIcon80(const QString &icon);
    QString maemoPngIcon80() const;
    void setSymbianTargetUid(const QString &uid);
    QString symbianTargetUid() const;
    void setNetworkEnabled(bool enabled);
    bool networkEnabled() const;
    QString path(int fileType) const;
    QString error() const;

    bool canSupportMeegoBooster() const;
    bool supportsMeegoBooster() const;
    void setSupportsMeegoBooster(bool supportBooster);

#ifndef CREATORLESSTEST
    virtual Core::GeneratedFiles generateFiles(QString *errorMessage) const;
#else
    bool generateFiles(QString *errorMessage) const;
#endif // CREATORLESSTEST

    static QString symbianUidForPath(const QString &path);
    static int makeStubVersion(int minor);
    QList<AbstractGeneratedFileInfo> fileUpdates(const QString &mainProFile) const;
    bool updateFiles(const QList<AbstractGeneratedFileInfo> &list, QString &error) const;

    static const QString DeploymentPriFileName;
protected:
    AbstractMobileApp();

    static QString templatesRoot();
    static void insertParameter(QString &line, const QString &parameter);

    QByteArray readBlob(const QString &filePath, QString *errorMsg) const;
    bool readTemplate(int fileType, QByteArray *data, QString *errorMessage) const;
    QByteArray generateFile(int fileType, QString *errorMessage) const;
    QString outputPathBase() const;

#ifndef CREATORLESSTEST
    static Core::GeneratedFile file(const QByteArray &data,
        const QString &targetFile);
#endif // CREATORLESSTEST

    static const QString CFileComment;
    static const QString ProFileComment;
    static const QString FileChecksum;
    static const QString FileStubVersion;
    static const int StubVersion;

    QString m_error;
    bool m_canSupportMeegoBooster;

private:
    QByteArray generateDesktopFile(QString *errorMessage, int fileType) const;
    QByteArray generateMainCpp(QString *errorMessage) const;
    QByteArray generateProFile(QString *errorMessage) const;

    virtual QByteArray generateFileExtended(int fileType,
        bool *versionAndCheckSum, QString *comment, QString *errorMessage) const=0;
    virtual QString pathExtended(int fileType) const=0;
    virtual QString originsRoot() const=0;
    virtual QString mainWindowClassName() const=0;
    virtual int stubVersionMinor() const=0;
    virtual bool adaptCurrentMainCppTemplateLine(QString &line) const=0;
    virtual void handleCurrentProFileTemplateLine(const QString &line,
        QTextStream &proFileTemplate, QTextStream &proFile,
        bool &commentOutNextLine) const = 0;
    virtual QList<AbstractGeneratedFileInfo> updateableFiles(const QString &mainProFile) const = 0;
    virtual QList<DeploymentFolder> deploymentFolders() const = 0;

    QString m_projectName;
    QFileInfo m_projectPath;
    QString m_symbianSvgIcon;
    QString m_maemoPngIcon64;
    QString m_maemoPngIcon80;
    QString m_symbianTargetUid;
    ScreenOrientation m_orientation;
    bool m_networkEnabled;
    bool m_supportsMeegoBooster;
};

} // namespace Qt4ProjectManager

#endif // ABSTRACTMOBILEAPP_H
