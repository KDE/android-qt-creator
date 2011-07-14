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

#include "testwizard.h"
#include "testwizarddialog.h"

#include <cpptools/abstracteditorsupport.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>

#include <QtGui/QIcon>

namespace Qt4ProjectManager {
namespace Internal {

TestWizard::TestWizard() :
    QtWizard(QLatin1String("L.Qt4Test"),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_CATEGORY),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_TR_SCOPE),
             QLatin1String(ProjectExplorer::Constants::PROJECT_WIZARD_TR_CATEGORY),
             tr("Qt Unit Test"),
             tr("Creates a QTestLib-based unit test for a feature or a class. "
                "Unit tests allow you to verify that the code is fit for use "
                "and that there are no regressions."),
             QIcon(QLatin1String(":/wizards/images/console.png")))
{
}

QWizard *TestWizard::createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const
{
    TestWizardDialog *dialog = new TestWizardDialog(displayName(), icon(), extensionPages, parent);
    dialog->setPath(defaultPath);
    dialog->setProjectName(TestWizardDialog::uniqueProjectName(defaultPath));
    return dialog;
}

// ---------------- code generation helpers
static const char initTestCaseC[] = "initTestCase";
static const char cleanupTestCaseC[] = "cleanupTestCase";
static const char closeFunctionC[] = "}\n\n";
static const char testDataTypeC[] = "QString";

static inline void writeVoidMemberDeclaration(QTextStream &str,
                                              const QString &indent,
                                              const QString &methodName)
{
    str << indent << "void " << methodName << "();\n";
}

static inline void writeVoidMemberBody(QTextStream &str,
                                       const QString &className,
                                       const QString &methodName,
                                       bool close = true)
{
    str << "void " << className << "::" << methodName << "()\n{\n";
    if (close)
        str << closeFunctionC;
}

static QString generateTestCode(const TestWizardParameters &testParams,
                                const QString &sourceBaseName)
{
    QString rc;
    const QString indent = QString(4, QLatin1Char(' '));
    QTextStream str(&rc);
    // Includes
    str << CppTools::AbstractEditorSupport::licenseTemplate(testParams.fileName, testParams.className)
        << "#include <QtCore/QString>\n#include <QtTest/QtTest>\n";
    if (testParams.requiresQApplication)
        str << "#include <QtCore/QCoreApplication>\n";
    // Class declaration
    str  << "\nclass " << testParams.className << " : public QObject\n"
        "{\n" << indent << "Q_OBJECT\n\npublic:\n"
        << indent << testParams.className << "();\n\nprivate Q_SLOTS:\n";
    if (testParams.initializationCode) {
        writeVoidMemberDeclaration(str, indent, QLatin1String(initTestCaseC));
        writeVoidMemberDeclaration(str, indent, QLatin1String(cleanupTestCaseC));
    }
    const QString dataSlot = testParams.testSlot + QLatin1String("_data");
    writeVoidMemberDeclaration(str, indent, testParams.testSlot);
    if (testParams.useDataSet)
        writeVoidMemberDeclaration(str, indent, dataSlot);
    str << "};\n\n";
    // Code: Constructor
    str << testParams.className << "::" << testParams.className << "()\n{\n}\n\n";
    // Code: Initialization slots
    if (testParams.initializationCode) {
        writeVoidMemberBody(str, testParams.className, QLatin1String(initTestCaseC));
        writeVoidMemberBody(str, testParams.className, QLatin1String(cleanupTestCaseC));
    }
    // Test slot with data or dummy
    writeVoidMemberBody(str, testParams.className, testParams.testSlot, false);
    if (testParams.useDataSet) {
        str << indent << "QFETCH(" << testDataTypeC << ", data);\n";
    }
    switch (testParams.type) {
    case TestWizardParameters::Test:
        str << indent << "QVERIFY2(true, \"Failure\");\n";
        break;
    case TestWizardParameters::Benchmark:
        str << indent << "QBENCHMARK {\n" << indent << "}\n";
        break;
    }
    str << closeFunctionC;
    // test data generation slot
    if (testParams.useDataSet) {
        writeVoidMemberBody(str, testParams.className, dataSlot, false);
        str << indent << "QTest::addColumn<" << testDataTypeC << ">(\"data\");\n"
            << indent << "QTest::newRow(\"0\") << " << testDataTypeC << "();\n"
            << closeFunctionC;
    }
    // Main & moc include
    str << (testParams.requiresQApplication ? "QTEST_MAIN" : "QTEST_APPLESS_MAIN")
        << '(' << testParams.className << ");\n\n"
        << "#include \"" << sourceBaseName << ".moc\"\n";
    return rc;
}

Core::GeneratedFiles TestWizard::generateFiles(const QWizard *w, QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const TestWizardDialog *wizardDialog = qobject_cast<const TestWizardDialog *>(w);
    QTC_ASSERT(wizardDialog, return Core::GeneratedFiles())

    const QtProjectParameters projectParams = wizardDialog->projectParameters();
    const TestWizardParameters testParams = wizardDialog->testParameters();
    const QString projectPath = projectParams.projectPath();

    // Create files: Source
    const QString sourceFilePath = Core::BaseFileWizard::buildFileName(projectPath, testParams.fileName, sourceSuffix());
    const QFileInfo sourceFileInfo(sourceFilePath);

    Core::GeneratedFile source(sourceFilePath);
    source.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    source.setContents(generateTestCode(testParams, sourceFileInfo.baseName()));

    // Create profile with define for base dir to find test data
    const QString profileName = Core::BaseFileWizard::buildFileName(projectPath, projectParams.fileName, profileSuffix());
    Core::GeneratedFile profile(profileName);
    profile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    QString contents;
    {
        QTextStream proStr(&contents);
        QtProjectParameters::writeProFileHeader(proStr);
        projectParams.writeProFile(proStr);
        proStr << "\n\nSOURCES += " << QFileInfo(sourceFilePath).fileName() << '\n'
               << "DEFINES += SRCDIR=\\\\\\\"$$PWD/\\\\\\\"\n";
    }
    profile.setContents(contents);

    return Core::GeneratedFiles() <<  source << profile;
}

} // namespace Internal
} // namespace Qt4ProjectManager
