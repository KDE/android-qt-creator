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

#include <QtCore/QString>
#include <QtTest/QtTest>

#include <coreplugin/externaltool.h>

using namespace Core::Internal;

static const char TEST_XML1[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<externaltool id=\"lupdate\">"
"    <description>Synchronizes translator's ts files with the program code</description>"
"    <description xml:lang=\"de\">Synchronisiert die ts-Übersetzungsdateien mit dem Programmcode</description>"
"    <displayname>Update translations (lupdate)</displayname>"
"    <displayname xml:lang=\"de\">Übersetzungen aktualisieren (lupdate)</displayname>"
"    <category>Linguist</category>"
"    <category xml:lang=\"de\">Linguist</category>"
"    <order>1</order>"
"    <executable error=\"ignore\">"
"        <path>%{QT_INSTALL_BINS}/lupdate</path>"
"        <path>lupdate</path>"
"        <arguments>%{CurrentProjectFilePath}</arguments>"
"        <workingdirectory>%{CurrentProjectPath}</workingdirectory>"
"    </executable>"
"</externaltool>"
;

static const char TEST_XML2[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<externaltool id=\"sort\">"
"    <description>Sorts the selected text</description>"
"    <description xml:lang=\"de\">Sortiert den ausgewählten Text</description>"
"    <displayname>Sort</displayname>"
"    <displayname xml:lang=\"de\">Sortieren</displayname>"
"    <category>Text</category>"
"    <category xml:lang=\"de\">Text</category>"
"    <executable output=\"replaceselection\">"
"        <path>sort</path>"
"        <input>%{CurrentSelection}</input>"
"        <workingdirectory>%{CurrentPath}</workingdirectory>"
"    </executable>"
"</externaltool>";

static const char TEST_XML3[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<externaltool id=\"vi\">"
"    <description>Opens the current file in vi</description>"
"    <description xml:lang=\"de\">Öffnet die aktuelle Datei in vi</description>"
"    <displayname>Edit with vi</displayname>"
"    <displayname xml:lang=\"de\">In vi öffnen</displayname>"
"    <category>Text</category>"
"    <category xml:lang=\"de\">Text</category>"
"    <executable modifiesdocument=\"yes\">"
"        <path>xterm</path>"
"        <arguments>-geom %{EditorCharWidth}x%{EditorCharHeight}+%{EditorXPos}+%{EditorYPos} -e vi %{CurrentFilePath} +%{EditorLine} +\"normal %{EditorColumn}|\"</arguments>"
"        <workingdirectory>%{CurrentPath}</workingdirectory>"
"    </executable>"
"</externaltool>";

static const char TEST_XML_LANG[] =
"<?xml version=\"1.0\" encoding=\"Latin-1\"?>"
"<externaltool id=\"temp\">"
"    <description>Hi</description>"
"    <description xml:lang=\"de\">Hallo</description>"
"    <description xml:lang=\"de_CH\">Grüezi</description>"
"    <displayname xml:lang=\"de\">Hallo</displayname>"
"    <displayname>Hi</displayname>"
"    <displayname xml:lang=\"de_CH\">Grüezi</displayname>"
"    <category xml:lang=\"de_CH\">Grüezi</category>"
"    <category>Hi</category>"
"    <category xml:lang=\"de\">Hallo</category>"
"    <executable>"
"        <path>foo</path>"
"    </executable>"
"</externaltool>";

class ExternaltoolTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testRead1();
    void testRead2();
    void testRead3();
    void testReadLocale();
};

void ExternaltoolTest::testRead1()
{
    QString error;
    ExternalTool *tool = ExternalTool::createFromXml(QByteArray(TEST_XML1), &error);
    QVERIFY(tool != 0);
    QVERIFY(error.isEmpty());
    QCOMPARE(tool->id(), QString::fromLatin1("lupdate"));
    QVERIFY(tool->description().startsWith(QLatin1String("Synchronizes tran")));
    QCOMPARE(tool->displayName(), QString::fromLatin1("Update translations (lupdate)"));
    QCOMPARE(tool->displayCategory(), QString::fromLatin1("Linguist"));
    QCOMPARE(tool->order(), 1);
    QCOMPARE(tool->executables().size(), 2);
    QCOMPARE(tool->executables().at(0), QString::fromLatin1("%{QT_INSTALL_BINS}/lupdate"));
    QCOMPARE(tool->executables().at(1), QString::fromLatin1("lupdate"));
    QCOMPARE(tool->arguments(), QString::fromLatin1("%{CurrentProjectFilePath}"));
    QCOMPARE(tool->input(), QString());
    QCOMPARE(tool->workingDirectory(), QString::fromLatin1("%{CurrentProjectPath}"));
    QCOMPARE(tool->outputHandling(), ExternalTool::ShowInPane);
    QCOMPARE(tool->errorHandling(), ExternalTool::Ignore);
    delete tool;
}

void ExternaltoolTest::testRead2()
{
    QString error;
    ExternalTool *tool = ExternalTool::createFromXml(QByteArray(TEST_XML2), &error);
    QVERIFY(tool != 0);
    QVERIFY(error.isEmpty());
    QCOMPARE(tool->id(), QString::fromLatin1("sort"));
    QVERIFY(tool->description().startsWith(QLatin1String("Sorts the")));
    QCOMPARE(tool->displayName(), QString::fromLatin1("Sort"));
    QCOMPARE(tool->displayCategory(), QString::fromLatin1("Text"));
    QCOMPARE(tool->order(), -1);
    QCOMPARE(tool->executables().size(), 1);
    QCOMPARE(tool->executables().at(0), QString::fromLatin1("sort"));
    QCOMPARE(tool->arguments(), QString());
    QCOMPARE(tool->input(), QString::fromLatin1("%{CurrentSelection}"));
    QCOMPARE(tool->workingDirectory(), QString::fromLatin1("%{CurrentPath}"));
    QCOMPARE(tool->outputHandling(), ExternalTool::ReplaceSelection);
    QCOMPARE(tool->errorHandling(), ExternalTool::ShowInPane);
    delete tool;
}

void ExternaltoolTest::testRead3()
{
    QString error;
    ExternalTool *tool = ExternalTool::createFromXml(QByteArray(TEST_XML3), &error);
    QVERIFY(tool != 0);
    QVERIFY(error.isEmpty());
    QCOMPARE(tool->id(), QString::fromLatin1("vi"));
    QVERIFY(tool->description().startsWith(QLatin1String("Opens the")));
    QCOMPARE(tool->displayName(), QString::fromLatin1("Edit with vi"));
    QCOMPARE(tool->displayCategory(), QString::fromLatin1("Text"));
    QCOMPARE(tool->order(), -1);
    QCOMPARE(tool->executables().size(), 1);
    QCOMPARE(tool->executables().at(0), QString::fromLatin1("xterm"));
    QVERIFY(tool->arguments().startsWith(QLatin1String("-geom %{")));
    QCOMPARE(tool->input(), QString());
    QCOMPARE(tool->workingDirectory(), QString::fromLatin1("%{CurrentPath}"));
    QCOMPARE(tool->outputHandling(), ExternalTool::ShowInPane);
    QCOMPARE(tool->modifiesCurrentDocument(), true);
    QCOMPARE(tool->errorHandling(), ExternalTool::ShowInPane);
    delete tool;
}

void ExternaltoolTest::testReadLocale()
{
    QString error;
    ExternalTool *tool;

    tool = ExternalTool::createFromXml(QByteArray(TEST_XML_LANG), &error);
    QVERIFY(tool != 0);
    QVERIFY(error.isEmpty());
    QCOMPARE(tool->description(), QString::fromLatin1("Hi"));
    QCOMPARE(tool->displayName(), QString::fromLatin1("Hi"));
    QCOMPARE(tool->displayCategory(), QString::fromLatin1("Hi"));
    delete tool;

    tool = ExternalTool::createFromXml(QByteArray(TEST_XML_LANG), &error, QLatin1String("uk"));
    QVERIFY(tool != 0);
    QVERIFY(error.isEmpty());
    QCOMPARE(tool->description(), QString::fromLatin1("Hi"));
    QCOMPARE(tool->displayName(), QString::fromLatin1("Hi"));
    QCOMPARE(tool->displayCategory(), QString::fromLatin1("Hi"));
    delete tool;

    tool = ExternalTool::createFromXml(QByteArray(TEST_XML_LANG), &error, QLatin1String("de_DE.UTF-8"));
    QVERIFY(tool != 0);
    QVERIFY(error.isEmpty());
    QCOMPARE(tool->description(), QString::fromLatin1("Hallo"));
    QCOMPARE(tool->displayName(), QString::fromLatin1("Hallo"));
    QCOMPARE(tool->displayCategory(), QString::fromLatin1("Hallo"));
    delete tool;

    tool = ExternalTool::createFromXml(QByteArray(TEST_XML_LANG), &error, QLatin1String("de_CH"));
    QVERIFY(tool != 0);
    QVERIFY(error.isEmpty());
    QCOMPARE(tool->description(), QString::fromLatin1("Grüezi"));
    QCOMPARE(tool->displayName(), QString::fromLatin1("Grüezi"));
    QCOMPARE(tool->displayCategory(), QString::fromLatin1("Grüezi"));
    delete tool;}

QTEST_APPLESS_MAIN(ExternaltoolTest);

#include "tst_externaltooltest.moc"
