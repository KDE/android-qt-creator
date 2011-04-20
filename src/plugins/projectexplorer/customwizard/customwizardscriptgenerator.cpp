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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "customwizardscriptgenerator.h"
#include "customwizard.h"
#include "customwizardparameters.h"

#include <utils/qtcassert.h>

#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QTemporaryFile>
#include <QtCore/QSharedPointer>

namespace ProjectExplorer {
namespace Internal {

// Parse helper: Determine the correct binary to run:
// Expand to full wizard path if it is relative and located
// in the wizard directory, else assume it can be found in path.
// On Windows, run non-exe files with 'cmd /c'.
QStringList fixGeneratorScript(const QString &configFile, QString binary)
{
    if (binary.isEmpty())
        return QStringList();
    // Expand to full path if it is relative and in the wizard
    // directory, else assume it can be found in path.
    QFileInfo binaryInfo(binary);
    if (!binaryInfo.isAbsolute()) {
        QString fullPath = QFileInfo(configFile).absolutePath();
        fullPath += QLatin1Char('/');
        fullPath += binary;
        const QFileInfo fullPathInfo(fullPath);
        if (fullPathInfo.isFile()) {
            binary = fullPathInfo.absoluteFilePath();
            binaryInfo = fullPathInfo;
        }
    } // not absolute
    QStringList rc(binary);
#ifdef Q_OS_WIN // Windows: Cannot run scripts by QProcess, do 'cmd /c'
    const QString extension = binaryInfo.suffix();
    if (!extension.isEmpty() && extension.compare(QLatin1String("exe"), Qt::CaseInsensitive) != 0) {
        rc.push_front(QLatin1String("/C"));
        rc.push_front(QString::fromLocal8Bit(qgetenv("COMSPEC")));
        if (rc.front().isEmpty())
            rc.front() = QLatin1String("cmd.exe");
    }
#endif
    return rc;
}

// Helper for running the optional generation script.
static bool
    runGenerationScriptHelper(const QString &workingDirectory,
                              const QStringList &script,
                              const QList<GeneratorScriptArgument> &argumentsIn,
                              bool dryRun,
                              const QMap<QString, QString> &fieldMap,
                              QString *stdOut /* = 0 */, QString *errorMessage)
{
    typedef QSharedPointer<QTemporaryFile> TemporaryFilePtr;
    typedef QList<TemporaryFilePtr> TemporaryFilePtrList;

    QProcess process;
    const QString binary = script.front();
    QStringList arguments;
    const int binarySize = script.size();
    for (int i = 1; i < binarySize; i++)
        arguments.push_back(script.at(i));

    // Arguments: Prepend 'dryrun' and do field replacement
    if (dryRun)
        arguments.push_back(QLatin1String("--dry-run"));

    // Arguments: Prepend 'dryrun'. Do field replacement to actual
    // argument value to expand via temporary file if specified
    CustomWizardContext::TemporaryFilePtrList temporaryFiles;
    foreach (const GeneratorScriptArgument &argument, argumentsIn) {
        QString value = argument.value;
        const bool nonEmptyReplacements
                = argument.flags & GeneratorScriptArgument::WriteFile ?
                    CustomWizardContext::replaceFields(fieldMap, &value, &temporaryFiles) :
                    CustomWizardContext::replaceFields(fieldMap, &value);
        if (nonEmptyReplacements || !(argument.flags & GeneratorScriptArgument::OmitEmpty))
            arguments.push_back(value);
    }
    process.setWorkingDirectory(workingDirectory);
    if (CustomWizard::verbose())
        qDebug("In %s, running:\n%s\n%s\n", qPrintable(workingDirectory),
               qPrintable(binary),
               qPrintable(arguments.join(QString(QLatin1Char(' ')))));
    process.start(binary, arguments);
    if (!process.waitForStarted()) {
        *errorMessage = QString::fromLatin1("Unable to start generator script %1: %2").
                arg(binary, process.errorString());
        return false;
    }
    if (!process.waitForFinished()) {
        *errorMessage = QString::fromLatin1("Generator script %1 timed out").arg(binary);
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *errorMessage = QString::fromLatin1("Generator script %1 crashed").arg(binary);
        return false;
    }
    if (process.exitCode() != 0) {
        const QString stdErr = QString::fromLocal8Bit(process.readAllStandardError());
        *errorMessage = QString::fromLatin1("Generator script %1 returned %2 (%3)").
                arg(binary).arg(process.exitCode()).arg(stdErr);
        return false;
    }
    if (stdOut) {
        *stdOut = QString::fromLocal8Bit(process.readAllStandardOutput());
        stdOut->remove(QLatin1Char('\r'));
        if (CustomWizard::verbose())
            qDebug("Output: '%s'\n", qPrintable(*stdOut));
    }
    return true;
}

// Do a dry run of the generation script to get a list of files
Core::GeneratedFiles
    dryRunCustomWizardGeneratorScript(const QString &targetPath,
                                      const QStringList &script,
                                      const QList<GeneratorScriptArgument> &arguments,
                                      const QMap<QString, QString> &fieldMap,
                                      QString *errorMessage)
{
    // Run in temporary directory as the target path may not exist yet.
    QString stdOut;
    if (!runGenerationScriptHelper(QDir::tempPath(), script, arguments, true,
                             fieldMap, &stdOut, errorMessage))
        return Core::GeneratedFiles();
    Core::GeneratedFiles files;
    // Parse the output consisting of lines with ',' separated tokens.
    // (file name + attributes matching those of the <file> element)
    foreach (const QString &line, stdOut.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            Core::GeneratedFile file;
            Core::GeneratedFile::Attributes attributes = Core::GeneratedFile::CustomGeneratorAttribute;
            const QStringList tokens = line.split(QLatin1Char(','));
            const int count = tokens.count();
            for (int i = 0; i < count; i++) {
                const QString &token = tokens.at(i);
                if (i) {
                    if (token == QLatin1String(customWizardFileOpenEditorAttributeC))
                        attributes |= Core::GeneratedFile::OpenEditorAttribute;
                    else if (token == QLatin1String(customWizardFileOpenProjectAttributeC))
                            attributes |= Core::GeneratedFile::OpenProjectAttribute;
                } else {
                    // Token 0 is file name. Wizard wants native names.
                    // Expand to full path if relative
                    const QFileInfo fileInfo(token);
                    const QString fullPath =
                            fileInfo.isAbsolute() ?
                            token :
                            (targetPath + QLatin1Char('/') + token);
                    file.setPath(fullPath);
                }
            }
            file.setAttributes(attributes);
            files.push_back(file);
        }
    }
    if (CustomWizard::verbose()) {
        QDebug nospace = qDebug().nospace();
        nospace << script << " generated:\n";
        foreach(const Core::GeneratedFile &f, files)
            nospace << ' ' << f.path() << f.attributes() << '\n';
    }
    return files;
}

bool runCustomWizardGeneratorScript(const QString &targetPath,
                                    const QStringList &script,
                                    const QList<GeneratorScriptArgument> &arguments,
                                    const QMap<QString, QString> &fieldMap,
                                    QString *errorMessage)
{
    return runGenerationScriptHelper(targetPath, script, arguments,
                                     false, fieldMap,
                                     0, errorMessage);
}

} // namespace Internal
} // namespace ProjectExplorer
