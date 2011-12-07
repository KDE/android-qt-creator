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

#include "msvctoolchain.h"

#include "msvcparser.h"
#include "projectexplorerconstants.h"
#include "headerpath.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>

#include <utils/fileutils.h>
#include <utils/synchronousprocess.h>
#include <utils/winutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtGui/QFormLayout>
#include <QtGui/QDesktopServices>

#define KEY_ROOT "ProjectExplorer.MsvcToolChain."
static const char debuggerCommandKeyC[] = KEY_ROOT"Debugger";
static const char varsBatKeyC[] = KEY_ROOT"VarsBat";
static const char varsBatArgKeyC[] = KEY_ROOT"VarsBatArg";
static const char supportedAbiKeyC[] = KEY_ROOT"SupportedAbi";

enum { debug = 0 };

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// Helpers:
// --------------------------------------------------------------------------

static QString platformName(MsvcToolChain::Platform t)
{
    switch (t) {
    case MsvcToolChain::s32:
        return QLatin1String(" (x86)");
    case MsvcToolChain::s64:
        return QLatin1String(" (x64)");
    case MsvcToolChain::ia64:
        return QLatin1String(" (ia64)");
    case MsvcToolChain::amd64:
        return QLatin1String(" (amd64)");
    }
    return QString();
}

static Abi findAbiOfMsvc(MsvcToolChain::Type type, MsvcToolChain::Platform platform, const QString &version)
{
    Abi::Architecture arch = Abi::X86Architecture;
    Abi::OSFlavor flavor = Abi::UnknownFlavor;
    int wordWidth = 64;

    switch (platform)
    {
    case ProjectExplorer::Internal::MsvcToolChain::s32:
        wordWidth = 32;
        break;
    case ProjectExplorer::Internal::MsvcToolChain::ia64:
        arch = Abi::ItaniumArchitecture;
        break;
    case ProjectExplorer::Internal::MsvcToolChain::s64:
    case ProjectExplorer::Internal::MsvcToolChain::amd64:
        break;
    };

    QString msvcVersionString = version;
    if (type == MsvcToolChain::WindowsSDK) {
        if (version.startsWith(QLatin1String("7.")))
            msvcVersionString = QLatin1String("10.0");
        else if (version.startsWith(QLatin1String("6.1"))
                 || (version.startsWith(QLatin1String("6.0")) && version != QLatin1String("6.0")))
            // The 6.0 SDK is shipping MSVC2005, Starting at 6.0a it is MSVC2008.
            msvcVersionString = QLatin1String("9.0");
        else
            msvcVersionString = QLatin1String("8.0");
    }
    if (msvcVersionString.startsWith(QLatin1String("10.")))
        flavor = Abi::WindowsMsvc2010Flavor;
    else if (msvcVersionString.startsWith(QLatin1String("9.")))
        flavor = Abi::WindowsMsvc2008Flavor;
    else
        flavor = Abi::WindowsMsvc2005Flavor;

    return Abi(arch, Abi::WindowsOS, flavor, Abi::PEFormat, wordWidth);
}

static QString generateDisplayName(const QString &name,
                                   MsvcToolChain::Type t,
                                   MsvcToolChain::Platform p)
{
    if (t == MsvcToolChain::WindowsSDK) {
        QString sdkName = name;
        sdkName += platformName(p);
        return sdkName;
    }
    // Comes as "9.0" from the registry
    QString vcName = QLatin1String("Microsoft Visual C++ Compiler ");
    vcName += name;
    vcName += platformName(p);
    return vcName;
}

static QByteArray msvcCompilationFile()
{
    static const char* macros[] = {"_ATL_VER", "_CHAR_UNSIGNED", "__CLR_VER",
                                   "__cplusplus_cli", "__COUNTER__", "__cplusplus",
                                   "_CPPLIB_VER", "_CPPRTTI", "_CPPUNWIND",
                                   "_DEBUG", "_DLL", "__FUNCDNAME__",
                                   "__FUNCSIG__","__FUNCTION__","_INTEGRAL_MAX_BITS",
                                   "_M_ALPHA","_M_CEE","_M_CEE_PURE",
                                   "_M_CEE_SAFE","_M_IX86","_M_IA64",
                                   "_M_IX86_FP","_M_MPPC","_M_MRX000",
                                   "_M_PPC","_M_X64","_MANAGED",
                                   "_MFC_VER","_MSC_BUILD", /* "_MSC_EXTENSIONS", */
                                   "_MSC_FULL_VER","_MSC_VER","__MSVC_RUNTIME_CHECKS",
                                   "_MT", "_NATIVE_WCHAR_T_DEFINED", "_OPENMP",
                                   "_VC_NODEFAULTLIB", "_WCHAR_T_DEFINED", "_WIN32",
                                   "_WIN32_WCE", "_WIN64", "_Wp64", "__DATE__",
                                   "__DATE__", "__TIME__", "__TIMESTAMP__",
                                   0};
    QByteArray file = "#define __PPOUT__(x) V##x=x\n\n";
    for (int i = 0; macros[i] != 0; ++i) {
        const QByteArray macro(macros[i]);
        file += "#if defined(" + macro + ")\n__PPOUT__("
                + macro + ")\n#endif\n";
    }
    file += "\nvoid main(){}\n\n";
    return file;
}

// Run MSVC 'cl' compiler to obtain #defines.
QByteArray MsvcToolChain::msvcPredefinedMacros(const Utils::Environment &env) const
{
    QByteArray predefinedMacros = AbstractMsvcToolChain::msvcPredefinedMacros(env);

    Utils::TempFileSaver saver(QDir::tempPath()+"/envtestXXXXXX.cpp");
    saver.write(msvcCompilationFile());
    if (!saver.finalize()) {
        qWarning("%s: %s", Q_FUNC_INFO, qPrintable(saver.errorString()));
        return predefinedMacros;
    }
    QProcess cpp;
    cpp.setEnvironment(env.toStringList());
    cpp.setWorkingDirectory(QDir::tempPath());
    QStringList arguments;
    const QString binary = env.searchInPath(QLatin1String("cl.exe"));
    if (binary.isEmpty()) {
        qWarning("%s: The compiler binary cl.exe could not be found in the path.", Q_FUNC_INFO);
        return predefinedMacros;
    }

    arguments << QLatin1String("/EP") << QDir::toNativeSeparators(saver.fileName());
    cpp.start(binary, arguments);
    if (!cpp.waitForStarted()) {
        qWarning("%s: Cannot start '%s': %s", Q_FUNC_INFO, qPrintable(binary),
            qPrintable(cpp.errorString()));
        return predefinedMacros;
    }
    cpp.closeWriteChannel();
    if (!cpp.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(cpp);
        qWarning("%s: Timeout running '%s'.", Q_FUNC_INFO, qPrintable(binary));
        return predefinedMacros;
    }
    if (cpp.exitStatus() != QProcess::NormalExit) {
        qWarning("%s: '%s' crashed.", Q_FUNC_INFO, qPrintable(binary));
        return predefinedMacros;
    }

    const QList<QByteArray> output = cpp.readAllStandardOutput().split('\n');
    foreach (const QByteArray& line, output) {
        if (line.startsWith('V')) {
            QList<QByteArray> split = line.split('=');
            const QByteArray key = split.at(0).mid(1);
            QByteArray value = split.at(1);
            if (!value.isEmpty()) {
                value.chop(1); //remove '\n'
            }
            predefinedMacros += "#define ";
            predefinedMacros += key;
            predefinedMacros += ' ';
            predefinedMacros += value;
            predefinedMacros += '\n';
        }
    }
    if (debug)
        qDebug() << "msvcPredefinedMacros" << predefinedMacros;
    return predefinedMacros;
}

// Windows: Expand the delayed evaluation references returned by the
// SDK setup scripts: "PATH=!Path!;foo". Some values might expand
// to empty and should not be added
static QString winExpandDelayedEnvReferences(QString in, const Utils::Environment &env)
{
    const QChar exclamationMark = QLatin1Char('!');
    for (int pos = 0; pos < in.size(); ) {
        // Replace "!REF!" by its value in process environment
        pos = in.indexOf(exclamationMark, pos);
        if (pos == -1)
            break;
        const int nextPos = in.indexOf(exclamationMark, pos + 1);
        if (nextPos == -1)
            break;
        const QString var = in.mid(pos + 1, nextPos - pos - 1);
        const QString replacement = env.value(var.toUpper());
        in.replace(pos, nextPos + 1 - pos, replacement);
        pos += replacement.size();
    }
    return in;
}

Utils::Environment MsvcToolChain::readEnvironmentSetting(Utils::Environment& env) const
{
    Utils::Environment result = env;
    if (!QFileInfo(m_vcvarsBat).exists())
        return result;

    QMap<QString, QString> envPairs;
    if (!generateEnvironmentSettings(env, m_vcvarsBat, m_varsBatArg, envPairs))
        return result;

    // Now loop through and process them
    QMap<QString,QString>::const_iterator envIter;
    for (envIter = envPairs.begin(); envIter!=envPairs.end(); ++envIter) {
        const QString expandedValue = winExpandDelayedEnvReferences(envIter.value(), env);
        if (!expandedValue.isEmpty())
            result.set(envIter.key(), expandedValue);
    }

    if (debug) {
        const QStringList newVars = result.toStringList();
        const QStringList oldVars = env.toStringList();
        QDebug nsp = qDebug().nospace();
        foreach (const QString &n, newVars) {
            if (!oldVars.contains(n))
                nsp << n << '\n';
        }
    }
    return result;
}

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

MsvcToolChain::MsvcToolChain(const QString &name, const Abi &abi,
                             const QString &varsBat, const QString &varsBatArg, bool autodetect) :
    AbstractMsvcToolChain(QLatin1String(Constants::MSVC_TOOLCHAIN_ID), autodetect, abi, varsBat),
    m_varsBatArg(varsBatArg)
{
    Q_ASSERT(!name.isEmpty());

    updateId();
    setDisplayName(name);
}

MsvcToolChain::MsvcToolChain() :
    AbstractMsvcToolChain(QLatin1String(Constants::MSVC_TOOLCHAIN_ID), false)
{
}

MsvcToolChain *MsvcToolChain::readFromMap(const QVariantMap &data)
{
    MsvcToolChain *tc = new MsvcToolChain;
    if (tc->fromMap(data))
        return tc;
    delete tc;
    return 0;
}

void MsvcToolChain::updateId()
{
    const QChar colon = QLatin1Char(':');
    QString id = QLatin1String(Constants::MSVC_TOOLCHAIN_ID);
    id += colon;
    id += m_vcvarsBat;
    id += colon;
    id += m_varsBatArg;
    id += colon;
    id += m_debuggerCommand;
    setId(id);
}

QString MsvcToolChain::typeName() const
{
    return MsvcToolChainFactory::tr("MSVC");
}

Utils::FileName MsvcToolChain::mkspec() const
{
    if (m_abi.osFlavor() == Abi::WindowsMsvc2005Flavor)
        return Utils::FileName::fromString(QLatin1String("win32-msvc2005"));
    if (m_abi.osFlavor() == Abi::WindowsMsvc2008Flavor)
        return Utils::FileName::fromString(QLatin1String("win32-msvc2008"));
    if (m_abi.osFlavor() == Abi::WindowsMsvc2010Flavor)
        return Utils::FileName::fromString(QLatin1String("win32-msvc2010"));
    return Utils::FileName();
}

QVariantMap MsvcToolChain::toMap() const
{
    QVariantMap data = ToolChain::toMap();
    if (!m_debuggerCommand.isEmpty())
        data.insert(QLatin1String(debuggerCommandKeyC), m_debuggerCommand);
    data.insert(QLatin1String(varsBatKeyC), m_vcvarsBat);
    if (!m_varsBatArg.isEmpty())
        data.insert(QLatin1String(varsBatArgKeyC), m_varsBatArg);
    data.insert(QLatin1String(supportedAbiKeyC), m_abi.toString());
    return data;
}

bool MsvcToolChain::fromMap(const QVariantMap &data)
{
    if (!ToolChain::fromMap(data))
        return false;
    m_vcvarsBat = data.value(QLatin1String(varsBatKeyC)).toString();
    m_varsBatArg = data.value(QLatin1String(varsBatArgKeyC)).toString();
    m_debuggerCommand = data.value(QLatin1String(debuggerCommandKeyC)).toString();
    const QString abiString = data.value(QLatin1String(supportedAbiKeyC)).toString();
    m_abi = Abi(abiString);
    updateId();

    return !m_vcvarsBat.isEmpty() && m_abi.isValid();
}


ToolChainConfigWidget *MsvcToolChain::configurationWidget()
{
    return new MsvcToolChainConfigWidget(this);
}

ToolChain *MsvcToolChain::clone() const
{
    return new MsvcToolChain(*this);
}

// --------------------------------------------------------------------------
// MsvcDebuggerConfigLabel
// --------------------------------------------------------------------------

static const char dgbToolsDownloadLink32C[] = "http://www.microsoft.com/whdc/devtools/debugging/installx86.Mspx";
static const char dgbToolsDownloadLink64C[] = "http://www.microsoft.com/whdc/devtools/debugging/install64bit.Mspx";

QString MsvcDebuggerConfigLabel::labelText()
{
#ifdef Q_OS_WIN
    const bool is64bit = Utils::winIs64BitSystem();
#else
    const bool is64bit = false;
#endif
    const QString link = is64bit ? QLatin1String(dgbToolsDownloadLink64C) : QLatin1String(dgbToolsDownloadLink32C);
    //: Label text for path configuration. %2 is "x-bit version".
    return tr(
    "<html><body><p>Specify the path to the "
    "<a href=\"%1\">Windows Console Debugger executable</a>"
    " (%2) here.</p>"
    "</body></html>").arg(link, (is64bit ? tr("64-bit version")
                                         : tr("32-bit version")));
}

MsvcDebuggerConfigLabel::MsvcDebuggerConfigLabel(QWidget *parent) :
        QLabel(labelText(), parent)
{
    connect(this, SIGNAL(linkActivated(QString)), this, SLOT(slotLinkActivated(QString)));
    setTextInteractionFlags(Qt::TextBrowserInteraction);
}

void MsvcDebuggerConfigLabel::slotLinkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

// --------------------------------------------------------------------------
// MsvcToolChainConfigWidget
// --------------------------------------------------------------------------

MsvcToolChainConfigWidget::MsvcToolChainConfigWidget(ToolChain *tc) :
    ToolChainConfigWidget(tc),
    m_varsBatDisplayLabel(new QLabel(this))
{
    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->addRow(new QLabel(tc->displayName()));
    m_varsBatDisplayLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    formLayout->addRow(tr("Initialization:"), m_varsBatDisplayLabel);
    formLayout->addRow(new MsvcDebuggerConfigLabel);
    addDebuggerCommandControls(formLayout, QStringList(QLatin1String("-version")));
    addDebuggerAutoDetection(this, SLOT(autoDetectDebugger()));
    addErrorLabel(formLayout);
    setFromToolChain();
}

void MsvcToolChainConfigWidget::apply()
{
    MsvcToolChain *tc = static_cast<MsvcToolChain *>(toolChain());
    QTC_ASSERT(tc, return; );
    tc->setDebuggerCommand(debuggerCommand());
}

void MsvcToolChainConfigWidget::setFromToolChain()
{
    MsvcToolChain *tc = static_cast<MsvcToolChain *>(toolChain());
    QTC_ASSERT(tc, return);
    QString varsBatDisplay = tc->varsBat();
    if (!tc->varsBatArg().isEmpty()) {
        varsBatDisplay += QLatin1Char(' ');
        varsBatDisplay += tc->varsBatArg();
    }
    m_varsBatDisplayLabel->setText(varsBatDisplay);
    setDebuggerCommand(tc->debuggerCommand());
}

bool MsvcToolChainConfigWidget::isDirty() const
{
    MsvcToolChain *tc = static_cast<MsvcToolChain *>(toolChain());
    QTC_ASSERT(tc, return false);
    return debuggerCommand() != tc->debuggerCommand();
}

void MsvcToolChainConfigWidget::autoDetectDebugger()
{
    clearErrorMessage();

    MsvcToolChain *tc = static_cast<MsvcToolChain *>(toolChain());
    QTC_ASSERT(tc, return);
    ProjectExplorer::Abi abi = tc->targetAbi();

    const QPair<QString, QString> cdbExecutables = MsvcToolChain::autoDetectCdbDebugger();
    QString debugger;
    if (abi.wordWidth() == 32) {
        if (cdbExecutables.first.isEmpty()) {
            setErrorMessage(tr("No CDB debugger detected (neither 32bit nor 64bit)."));
            return;
        }
        debugger = cdbExecutables.first;
    } else if (abi.wordWidth() == 64) {
        if (cdbExecutables.second.isEmpty()) {
            setErrorMessage(tr("No 64bit CDB debugger detected."));
            return;
        }
        debugger = cdbExecutables.second;
    }
    if (debugger != debuggerCommand()) {
        setDebuggerCommand(debugger);
        emitDirty();
    }
}

// --------------------------------------------------------------------------
// MsvcToolChainFactory
// --------------------------------------------------------------------------

QString MsvcToolChainFactory::displayName() const
{
    return tr("MSVC");
}

QString MsvcToolChainFactory::id() const
{
    return QLatin1String(Constants::MSVC_TOOLCHAIN_ID);
}

QList<ToolChain *> MsvcToolChainFactory::autoDetect()
{
    QList<ToolChain *> results;

#ifdef Q_OS_WIN
    // 1) Installed SDKs preferred over standalone Visual studio
    const QSettings sdkRegistry("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows",
                                QSettings::NativeFormat);
    const QString defaultSdkPath = sdkRegistry.value(QLatin1String("CurrentInstallFolder")).toString();
    if (!defaultSdkPath.isEmpty()) {
        foreach (const QString &sdkKey, sdkRegistry.childGroups()) {
            const QString name = sdkRegistry.value(sdkKey + QLatin1String("/ProductName")).toString();
            const QString version = sdkRegistry.value(sdkKey + QLatin1String("/ProductVersion")).toString();
            const QString folder = sdkRegistry.value(sdkKey + QLatin1String("/InstallationFolder")).toString();
            if (folder.isEmpty())
                continue;

            const QString sdkVcVarsBat = folder + QLatin1String("bin\\SetEnv.cmd");
            if (!QFileInfo(sdkVcVarsBat).exists())
                continue;
            QList<ToolChain *> tmp;

            tmp.append(new MsvcToolChain(generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::s32),
                                         findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::s32, version),
                                         sdkVcVarsBat, QLatin1String("/x86"), true));
            // Add all platforms
            tmp.append(new MsvcToolChain(generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::s64),
                                         findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::s64, version),
                                         sdkVcVarsBat, QLatin1String("/x64"), true));
            tmp.append(new MsvcToolChain(generateDisplayName(name, MsvcToolChain::WindowsSDK, MsvcToolChain::ia64),
                                         findAbiOfMsvc(MsvcToolChain::WindowsSDK, MsvcToolChain::ia64, version),
                                         sdkVcVarsBat, QLatin1String("/ia64"), true));
            // Make sure the default is front.
            if (folder == defaultSdkPath)
                results = tmp + results;
            else
                results += tmp;
        } // foreach
    }

    // 2) Installed MSVCs
    const QSettings vsRegistry(
#ifdef Q_OS_WIN64
                QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7"),
#else
                QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VC7"),
#endif
                QSettings::NativeFormat);
    foreach (const QString &vsName, vsRegistry.allKeys()) {
        // Scan for version major.minor
        const int dotPos = vsName.indexOf(QLatin1Char('.'));
        if (dotPos == -1)
            continue;

        const QString path = vsRegistry.value(vsName).toString();
        const int version = vsName.left(dotPos).toInt();
        // Check existence of various install scripts
        const QString vcvars32bat = path + QLatin1String("bin\\vcvars32.bat");
        if (QFileInfo(vcvars32bat).isFile())
            results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::s32),
                                             findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::s32, vsName),
                                             vcvars32bat, QString(), true));
        if (version >= 10) {
            // Just one common file
            const QString vcvarsAllbat = path + QLatin1String("vcvarsall.bat");
            if (QFileInfo(vcvarsAllbat).isFile()) {
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::s32),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::s32, vsName),
                                                 vcvarsAllbat, QLatin1String("x86"), true));
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::amd64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::amd64, vsName),
                                                 vcvarsAllbat, QLatin1String("amd64"), true));
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::s64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::s64, vsName),
                                                 vcvarsAllbat, QLatin1String("x64"), true));
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::ia64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::ia64, vsName),
                                                 vcvarsAllbat, QLatin1String("ia64"), true));
            } else {
                qWarning("Unable to find MSVC setup script %s in version %d", qPrintable(vcvarsAllbat), version);
            }
        } else {
            // Amd 64 is the preferred 64bit platform
            const QString vcvarsAmd64bat = path + QLatin1String("bin\\amd64\\vcvarsamd64.bat");
            if (QFileInfo(vcvarsAmd64bat).isFile())
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::amd64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::amd64, vsName),
                                                 vcvarsAmd64bat, QString(), true));
            const QString vcvarsAmd64bat2 = path + QLatin1String("bin\\vcvarsx86_amd64.bat");
            if (QFileInfo(vcvarsAmd64bat2).isFile())
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::amd64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::amd64, vsName),
                                                 vcvarsAmd64bat2, QString(), true));
            const QString vcvars64bat = path + QLatin1String("bin\\vcvars64.bat");
            if (QFileInfo(vcvars64bat).isFile())
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::s64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::s64, vsName),
                                                 vcvars64bat, QString(), true));
            const QString vcvarsIA64bat = path + QLatin1String("bin\\vcvarsx86_ia64.bat");
            if (QFileInfo(vcvarsIA64bat).isFile())
                results.append(new MsvcToolChain(generateDisplayName(vsName, MsvcToolChain::VS, MsvcToolChain::ia64),
                                                 findAbiOfMsvc(MsvcToolChain::VS, MsvcToolChain::ia64, vsName),
                                                 vcvarsIA64bat, QString(), true));
        }
    }
#endif
    if (!results.isEmpty()) { // Detect debugger
        const QPair<QString, QString> cdbDebugger = MsvcToolChain::autoDetectCdbDebugger();
        foreach (ToolChain *tc, results)
            static_cast<MsvcToolChain *>(tc)->setDebuggerCommand(tc->targetAbi().wordWidth() == 32 ? cdbDebugger.first : cdbDebugger.second);
    }

    return results;
}

QPair<QString, QString> MsvcToolChain::autoDetectCdbDebugger()
{
    QPair<QString, QString> result;
    QStringList cdbs;

    QStringList programDirs;
    programDirs.append(QString::fromLocal8Bit(qgetenv("ProgramFiles")));
    programDirs.append(QString::fromLocal8Bit(qgetenv("ProgramFiles(x86)")));
    programDirs.append(QString::fromLocal8Bit(qgetenv("ProgramW6432")));

    foreach (const QString &dirName, programDirs) {
        if (dirName.isEmpty())
            continue;
        QDir dir(dirName);
        foreach (const QFileInfo &fi, dir.entryInfoList(QStringList(QLatin1String("Debugging Tools for Windows*")),
                                                        QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString filePath = fi.absoluteFilePath() + QLatin1String("/cdb.exe");
            if (!cdbs.contains(filePath))
                cdbs.append(fi.absoluteFilePath() + QLatin1String("/cdb.exe"));
        }
    }

    foreach (const QString &cdb, cdbs) {
        QList<ProjectExplorer::Abi> abis = ProjectExplorer::Abi::abisOfBinary(cdb);
        if (abis.isEmpty())
            continue;
        if (abis.first().wordWidth() == 32)
            result.first = cdb;
        else if (abis.first().wordWidth() == 64)
            result.second = cdb;
    }

    // prefer 64bit debugger, even for 32bit binaries:
    if (!result.second.isEmpty())
        result.first = result.second;

    return result;
}

bool MsvcToolChainFactory::canRestore(const QVariantMap &data)
{
    return idFromMap(data).startsWith(QLatin1String(Constants::MSVC_TOOLCHAIN_ID) + QLatin1Char(':'));
}

} // namespace Internal
} // namespace ProjectExplorer
