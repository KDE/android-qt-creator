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

#include "cppcodestylesettingspage.h"
#include "cppcodestylepreferences.h"
#include "ui_cppcodestylesettingspage.h"
#include "cpptoolsconstants.h"
#include "cpptoolssettings.h"
#include "cppqtstyleindenter.h"
#include <texteditor/snippets/isnippetprovider.h>
#include <texteditor/fontsettings.h>
#include <texteditor/displaysettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/codestyleeditor.h>
#include <extensionsystem/pluginmanager.h>
#include <cppeditor/cppeditorconstants.h>
#include <coreplugin/icore.h>
#include <QtGui/QTextBlock>
#include <QtCore/QTextStream>

static const char *defaultCodeStyleSnippets[] = {
    "#include <math.h>\n"
    "\n"
    "class Complex\n"
    "    {\n"
    "public:\n"
    "    Complex(double re, double im)\n"
    "        : _re(re), _im(im)\n"
    "        {}\n"
    "    double modulus() const\n"
    "        {\n"
    "        return sqrt(_re * _re + _im * _im);\n"
    "        }\n"
    "private:\n"
    "    double _re;\n"
    "    double _im;\n"
    "    };\n"
    "\n"
    "void bar(int i)\n"
    "    {\n"
    "    static int counter = 0;\n"
    "    counter += i;\n"
    "    }\n"
    "\n"
    "namespace Foo\n"
    "    {\n"
    "    namespace Bar\n"
    "        {\n"
    "        void foo(int a, int b)\n"
    "            {\n"
    "            for (int i = 0; i < a; i++)\n"
    "                {\n"
    "                if (i < b)\n"
    "                    bar(i);\n"
    "                else\n"
    "                    {\n"
    "                    bar(i);\n"
    "                    bar(b);\n"
    "                    }\n"
    "                }\n"
    "            }\n"
    "        } // namespace Bar\n"
    "    } // namespace Foo\n"
    ,
    "#include <math.h>\n"
    "\n"
    "class Complex\n"
    "    {\n"
    "public:\n"
    "    Complex(double re, double im)\n"
    "        : _re(re), _im(im)\n"
    "        {}\n"
    "    double modulus() const\n"
    "        {\n"
    "        return sqrt(_re * _re + _im * _im);\n"
    "        }\n"
    "private:\n"
    "    double _re;\n"
    "    double _im;\n"
    "    };\n"
    "\n"
    "void bar(int i)\n"
    "    {\n"
    "    static int counter = 0;\n"
    "    counter += i;\n"
    "    }\n"
    "\n"
    "namespace Foo\n"
    "    {\n"
    "    namespace Bar\n"
    "        {\n"
    "        void foo(int a, int b)\n"
    "            {\n"
    "            for (int i = 0; i < a; i++)\n"
    "                {\n"
    "                if (i < b)\n"
    "                    bar(i);\n"
    "                else\n"
    "                    {\n"
    "                    bar(i);\n"
    "                    bar(b);\n"
    "                    }\n"
    "                }\n"
    "            }\n"
    "        } // namespace Bar\n"
    "    } // namespace Foo\n"
    ,
    "namespace Foo\n"
    "{\n"
    "namespace Bar\n"
    "{\n"
    "class FooBar\n"
    "    {\n"
    "public:\n"
    "    FooBar(int a)\n"
    "        : _a(a)\n"
    "        {}\n"
    "    int calculate() const\n"
    "        {\n"
    "        if (a > 10)\n"
    "            {\n"
    "            int b = 2 * a;\n"
    "            return a * b;\n"
    "            }\n"
    "        return -a;\n"
    "        }\n"
    "private:\n"
    "    int _a;\n"
    "    };\n"
    "}\n"
    "}\n"
    ,
    "#include \"bar.h\"\n"
    "\n"
    "int foo(int a)\n"
    "    {\n"
    "    switch (a)\n"
    "        {\n"
    "        case 1:\n"
    "            bar(1);\n"
    "            break;\n"
    "        case 2:\n"
    "            {\n"
    "            bar(2);\n"
    "            break;\n"
    "            }\n"
    "        case 3:\n"
    "        default:\n"
    "            bar(3);\n"
    "            break;\n"
    "        }\n"
    "    return 0;\n"
    "    }\n"
    ,
    "void foo() {\n"
    "    if (a &&\n"
    "        b)\n"
    "        c;\n"
    "\n"
    "    while (a ||\n"
    "           b)\n"
    "        break;\n"
    "    a = b +\n"
    "        c;\n"
    "    myInstance.longMemberName +=\n"
    "            foo;\n"
    "    myInstance.longMemberName += bar +\n"
    "                                 foo;\n"
    "}\n"
};

using namespace TextEditor;

namespace CppTools {

namespace Internal {

// ------------------ CppCodeStyleSettingsWidget

CppCodeStylePreferencesWidget::CppCodeStylePreferencesWidget(QWidget *parent)
    : QWidget(parent),
      m_preferences(0),
      m_ui(new Ui::CppCodeStyleSettingsPage),
      m_blockUpdates(false)
{
    m_ui->setupUi(this);
    m_ui->categoryTab->setProperty("_q_custom_style_disabled", true);

    m_previews << m_ui->previewTextEditGeneral << m_ui->previewTextEditContent
               << m_ui->previewTextEditBraces << m_ui->previewTextEditSwitch
               << m_ui->previewTextEditPadding;
    for (int i = 0; i < m_previews.size(); ++i) {
        m_previews[i]->setPlainText(defaultCodeStyleSnippets[i]);
    }

    TextEditor::TextEditorSettings *textEditorSettings = TextEditorSettings::instance();
    decorateEditors(textEditorSettings->fontSettings());
    connect(textEditorSettings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
       this, SLOT(decorateEditors(TextEditor::FontSettings)));

    setVisualizeWhitespace(true);

    connect(m_ui->tabSettingsWidget, SIGNAL(settingsChanged(TextEditor::TabSettings)),
       this, SLOT(slotTabSettingsChanged(TextEditor::TabSettings)));
    connect(m_ui->indentBlockBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentBlockBody, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentClassBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentNamespaceBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentEnumBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentNamespaceBody, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentSwitchLabels, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentCaseStatements, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentCaseBlocks, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentCaseBreak, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentAccessSpecifiers, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentDeclarationsRelativeToAccessSpecifiers, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentFunctionBody, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->indentFunctionBraces, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->extraPaddingConditions, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));
    connect(m_ui->alignAssignments, SIGNAL(toggled(bool)),
       this, SLOT(slotCodeStyleSettingsChanged()));

    m_ui->categoryTab->setCurrentIndex(0);

    m_ui->tabSettingsWidget->setFlat(true);
}

CppCodeStylePreferencesWidget::~CppCodeStylePreferencesWidget()
{
    delete m_ui;
}

void CppCodeStylePreferencesWidget::setCodeStyle(CppTools::CppCodeStylePreferences *codeStylePreferences)
{
    // code preferences
    m_preferences = codeStylePreferences;

    connect(m_preferences, SIGNAL(currentTabSettingsChanged(TextEditor::TabSettings)),
            this, SLOT(setTabSettings(TextEditor::TabSettings)));
    connect(m_preferences, SIGNAL(currentCodeStyleSettingsChanged(CppTools::CppCodeStyleSettings)),
            this, SLOT(setCodeStyleSettings(CppTools::CppCodeStyleSettings)));
    connect(m_preferences, SIGNAL(currentPreferencesChanged(TextEditor::ICodeStylePreferences*)),
            this, SLOT(slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences*)));

    setTabSettings(m_preferences->tabSettings());
    setCodeStyleSettings(m_preferences->codeStyleSettings(), false);
    slotCurrentPreferencesChanged(m_preferences->currentPreferences(), false);

    updatePreview();
}

CppCodeStyleSettings CppCodeStylePreferencesWidget::cppCodeStyleSettings() const
{
    CppCodeStyleSettings set;

    set.indentBlockBraces = m_ui->indentBlockBraces->isChecked();
    set.indentBlockBody = m_ui->indentBlockBody->isChecked();
    set.indentClassBraces = m_ui->indentClassBraces->isChecked();
    set.indentEnumBraces = m_ui->indentEnumBraces->isChecked();
    set.indentNamespaceBraces = m_ui->indentNamespaceBraces->isChecked();
    set.indentNamespaceBody = m_ui->indentNamespaceBody->isChecked();
    set.indentAccessSpecifiers = m_ui->indentAccessSpecifiers->isChecked();
    set.indentDeclarationsRelativeToAccessSpecifiers = m_ui->indentDeclarationsRelativeToAccessSpecifiers->isChecked();
    set.indentFunctionBody = m_ui->indentFunctionBody->isChecked();
    set.indentFunctionBraces = m_ui->indentFunctionBraces->isChecked();
    set.indentSwitchLabels = m_ui->indentSwitchLabels->isChecked();
    set.indentStatementsRelativeToSwitchLabels = m_ui->indentCaseStatements->isChecked();
    set.indentBlocksRelativeToSwitchLabels = m_ui->indentCaseBlocks->isChecked();
    set.indentControlFlowRelativeToSwitchLabels = m_ui->indentCaseBreak->isChecked();
    set.extraPaddingForConditionsIfConfusingAlign = m_ui->extraPaddingConditions->isChecked();
    set.alignAssignments = m_ui->alignAssignments->isChecked();

    return set;
}

void CppCodeStylePreferencesWidget::setTabSettings(const TextEditor::TabSettings &settings)
{
    m_ui->tabSettingsWidget->setTabSettings(settings);
}

void CppCodeStylePreferencesWidget::setCodeStyleSettings(const CppCodeStyleSettings &s, bool preview)
{
    const bool wasBlocked = m_blockUpdates;
    m_blockUpdates = true;
    m_ui->indentBlockBraces->setChecked(s.indentBlockBraces);
    m_ui->indentBlockBody->setChecked(s.indentBlockBody);
    m_ui->indentClassBraces->setChecked(s.indentClassBraces);
    m_ui->indentEnumBraces->setChecked(s.indentEnumBraces);
    m_ui->indentNamespaceBraces->setChecked(s.indentNamespaceBraces);
    m_ui->indentNamespaceBody->setChecked(s.indentNamespaceBody);
    m_ui->indentAccessSpecifiers->setChecked(s.indentAccessSpecifiers);
    m_ui->indentDeclarationsRelativeToAccessSpecifiers->setChecked(s.indentDeclarationsRelativeToAccessSpecifiers);
    m_ui->indentFunctionBody->setChecked(s.indentFunctionBody);
    m_ui->indentFunctionBraces->setChecked(s.indentFunctionBraces);
    m_ui->indentSwitchLabels->setChecked(s.indentSwitchLabels);
    m_ui->indentCaseStatements->setChecked(s.indentStatementsRelativeToSwitchLabels);
    m_ui->indentCaseBlocks->setChecked(s.indentBlocksRelativeToSwitchLabels);
    m_ui->indentCaseBreak->setChecked(s.indentControlFlowRelativeToSwitchLabels);
    m_ui->extraPaddingConditions->setChecked(s.extraPaddingForConditionsIfConfusingAlign);
    m_ui->alignAssignments->setChecked(s.alignAssignments);
    m_blockUpdates = wasBlocked;
    if (preview)
        updatePreview();
}

void CppCodeStylePreferencesWidget::slotCurrentPreferencesChanged(TextEditor::ICodeStylePreferences *preferences, bool preview)
{
    const bool enable = !preferences->isReadOnly() && !m_preferences->currentDelegate();
    m_ui->tabSettingsWidget->setEnabled(enable);
    m_ui->contentGroupBox->setEnabled(enable);
    m_ui->bracesGroupBox->setEnabled(enable);
    m_ui->switchGroupBox->setEnabled(enable);
    m_ui->alignmentGroupBox->setEnabled(enable);
    if (preview)
        updatePreview();
}

QString CppCodeStylePreferencesWidget::searchKeywords() const
{
    QString rc;
    QLatin1Char sep(' ');
    QTextStream(&rc)
       << sep << m_ui->tabSettingsWidget->searchKeywords()
       << sep << m_ui->indentBlockBraces->text()
       << sep << m_ui->indentBlockBody->text()
       << sep << m_ui->indentClassBraces->text()
       << sep << m_ui->indentEnumBraces->text()
       << sep << m_ui->indentNamespaceBraces->text()
       << sep << m_ui->indentNamespaceBody->text()
       << sep << m_ui->indentAccessSpecifiers->text()
       << sep << m_ui->indentDeclarationsRelativeToAccessSpecifiers->text()
       << sep << m_ui->indentFunctionBody->text()
       << sep << m_ui->indentFunctionBraces->text()
       << sep << m_ui->indentSwitchLabels->text()
       << sep << m_ui->indentCaseStatements->text()
       << sep << m_ui->indentCaseBlocks->text()
       << sep << m_ui->indentCaseBreak->text()
       << sep << m_ui->contentGroupBox->title()
       << sep << m_ui->bracesGroupBox->title()
       << sep << m_ui->switchGroupBox->title()
       << sep << m_ui->alignmentGroupBox->title()
       << sep << m_ui->extraPaddingConditions->text()
       << sep << m_ui->alignAssignments->text()
          ;
    for (int i = 0; i < m_ui->categoryTab->count(); i++)
        QTextStream(&rc) << sep << m_ui->categoryTab->tabText(i);
    rc.remove(QLatin1Char('&'));
    return rc;
}

void CppCodeStylePreferencesWidget::slotCodeStyleSettingsChanged()
{
    if (m_blockUpdates)
        return;

    if (m_preferences) {
        CppCodeStylePreferences *current = qobject_cast<CppCodeStylePreferences *>(m_preferences->currentPreferences());
        if (current)
            current->setCodeStyleSettings(cppCodeStyleSettings());
    }

    updatePreview();
}

void CppCodeStylePreferencesWidget::slotTabSettingsChanged(const TextEditor::TabSettings &settings)
{
    if (m_blockUpdates)
        return;

    if (m_preferences) {
        CppCodeStylePreferences *current = qobject_cast<CppCodeStylePreferences *>(m_preferences->currentPreferences());
        if (current)
            current->setTabSettings(settings);
    }

    updatePreview();
}

void CppCodeStylePreferencesWidget::updatePreview()
{
    CppCodeStylePreferences *cppCodeStylePreferences = m_preferences
            ? m_preferences
            : CppToolsSettings::instance()->cppCodeStyle();
    const CppCodeStyleSettings ccss = cppCodeStylePreferences->currentCodeStyleSettings();
    const TextEditor::TabSettings ts = cppCodeStylePreferences->currentTabSettings();
    QtStyleCodeFormatter formatter(ts, ccss);
    foreach (TextEditor::SnippetEditorWidget *preview, m_previews) {
        preview->setTabSettings(ts);
        preview->setCodeStyle(cppCodeStylePreferences);

        QTextDocument *doc = preview->document();
        formatter.invalidateCache(doc);

        QTextBlock block = doc->firstBlock();
        QTextCursor tc = preview->textCursor();
        tc.beginEditBlock();
        while (block.isValid()) {
            preview->indenter()->indentBlock(doc, block, QChar::Null, ts);

            block = block.next();
        }
        tc.endEditBlock();
    }
}

void CppCodeStylePreferencesWidget::decorateEditors(const TextEditor::FontSettings &fontSettings)
{
    const ISnippetProvider *provider = 0;
    const QList<ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::instance()->getObjects<ISnippetProvider>();
    foreach (const ISnippetProvider *current, providers) {
        if (current->groupId() == QLatin1String(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID)) {
            provider = current;
            break;
        }
    }

    foreach (TextEditor::SnippetEditorWidget *editor, m_previews) {
        editor->setFontSettings(fontSettings);
        if (provider)
            provider->decorateEditor(editor);
    }
}

void CppCodeStylePreferencesWidget::setVisualizeWhitespace(bool on)
{
    foreach (TextEditor::SnippetEditorWidget *editor, m_previews) {
        DisplaySettings displaySettings = editor->displaySettings();
        displaySettings.m_visualizeWhitespace = on;
        editor->setDisplaySettings(displaySettings);
    }
}


// ------------------ CppCodeStyleSettingsPage

CppCodeStyleSettingsPage::CppCodeStyleSettingsPage(
        QWidget *parent) :
    Core::IOptionsPage(parent),
    m_pageCppCodeStylePreferences(0)
{
}

CppCodeStyleSettingsPage::~CppCodeStyleSettingsPage()
{
}

QString CppCodeStyleSettingsPage::id() const
{
    return QLatin1String(Constants::CPP_CODE_STYLE_SETTINGS_ID);
}

QString CppCodeStyleSettingsPage::displayName() const
{
    return QCoreApplication::translate("CppTools", Constants::CPP_CODE_STYLE_SETTINGS_NAME);
}

QString CppCodeStyleSettingsPage::category() const
{
    return QLatin1String(Constants::CPP_SETTINGS_CATEGORY);
}

QString CppCodeStyleSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("CppTools", Constants::CPP_SETTINGS_TR_CATEGORY);
}

QIcon CppCodeStyleSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::SETTINGS_CATEGORY_CPP_ICON));
}

QWidget *CppCodeStyleSettingsPage::createPage(QWidget *parent)
{
    CppCodeStylePreferences *originalCodeStylePreferences
            = CppToolsSettings::instance()->cppCodeStyle();
    m_pageCppCodeStylePreferences = new CppCodeStylePreferences(m_widget);
    m_pageCppCodeStylePreferences->setDelegatingPool(originalCodeStylePreferences->delegatingPool());
    m_pageCppCodeStylePreferences->setCodeStyleSettings(originalCodeStylePreferences->codeStyleSettings());
    m_pageCppCodeStylePreferences->setCurrentDelegate(originalCodeStylePreferences->currentDelegate());
    // we set id so that it won't be possible to set delegate to the original prefs
    m_pageCppCodeStylePreferences->setId(originalCodeStylePreferences->id());
    TextEditorSettings *settings = TextEditorSettings::instance();
    m_widget = new CodeStyleEditor(settings->codeStyleFactory(CppTools::Constants::CPP_SETTINGS_ID),
                                   m_pageCppCodeStylePreferences, parent);

    return m_widget;
}

void CppCodeStyleSettingsPage::apply()
{
    if (m_widget) {
        QSettings *s = Core::ICore::instance()->settings();

        CppCodeStylePreferences *originalCppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStyle();
        if (originalCppCodeStylePreferences->codeStyleSettings() != m_pageCppCodeStylePreferences->codeStyleSettings()) {
            originalCppCodeStylePreferences->setCodeStyleSettings(m_pageCppCodeStylePreferences->codeStyleSettings());
            if (s)
                originalCppCodeStylePreferences->toSettings(CppTools::Constants::CPP_SETTINGS_ID, s);
        }
        if (originalCppCodeStylePreferences->tabSettings() != m_pageCppCodeStylePreferences->tabSettings()) {
            originalCppCodeStylePreferences->setTabSettings(m_pageCppCodeStylePreferences->tabSettings());
            if (s)
                originalCppCodeStylePreferences->toSettings(CppTools::Constants::CPP_SETTINGS_ID, s);
        }
        if (originalCppCodeStylePreferences->currentDelegate() != m_pageCppCodeStylePreferences->currentDelegate()) {
            originalCppCodeStylePreferences->setCurrentDelegate(m_pageCppCodeStylePreferences->currentDelegate());
            if (s)
                originalCppCodeStylePreferences->toSettings(CppTools::Constants::CPP_SETTINGS_ID, s);
        }
    }
}

bool CppCodeStyleSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace CppTools
