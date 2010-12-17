/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "behaviorsettingspage.h"

#include "behaviorsettings.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "ui_behaviorsettingspage.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QSettings>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>

using namespace TextEditor;

struct BehaviorSettingsPage::BehaviorSettingsPagePrivate
{
    explicit BehaviorSettingsPagePrivate(const BehaviorSettingsPageParameters &p);

    const BehaviorSettingsPageParameters m_parameters;
    Ui::BehaviorSettingsPage *m_page;

    TabSettings m_tabSettings;
    StorageSettings m_storageSettings;
    BehaviorSettings m_behaviorSettings;

    QString m_searchKeywords;
};

BehaviorSettingsPage::BehaviorSettingsPagePrivate::BehaviorSettingsPagePrivate
    (const BehaviorSettingsPageParameters &p)
    : m_parameters(p), m_page(0)
{
    if (const QSettings *s = Core::ICore::instance()->settings()) {
        m_tabSettings.fromSettings(m_parameters.settingsPrefix, s);
        m_storageSettings.fromSettings(m_parameters.settingsPrefix, s);
        m_behaviorSettings.fromSettings(m_parameters.settingsPrefix, s);
    }
}

BehaviorSettingsPage::BehaviorSettingsPage(const BehaviorSettingsPageParameters &p,
                                           QObject *parent)
  : TextEditorOptionsPage(parent),
    m_d(new BehaviorSettingsPagePrivate(p))
{
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
    delete m_d;
}

QString BehaviorSettingsPage::id() const
{
    return m_d->m_parameters.id;
}

QString BehaviorSettingsPage::displayName() const
{
    return m_d->m_parameters.displayName;
}

QWidget *BehaviorSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->m_page = new Ui::BehaviorSettingsPage;
    m_d->m_page->setupUi(w);
    settingsToUI();
    if (m_d->m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_d->m_searchKeywords)
          << m_d->m_page->insertSpaces->text()
          << sep << m_d->m_page->autoInsertSpaces->text()
          << sep << m_d->m_page->autoIndent->text()
          << sep << m_d->m_page->smartBackspace->text()
          << sep << m_d->m_page->indentBlocksLabel->text()
          << sep << m_d->m_page->continuationAlignLabel->text()
          << sep << m_d->m_page->tabKeyIndentLabel->text()
          << sep << m_d->m_page->cleanWhitespace->text()
          << sep << m_d->m_page->inEntireDocument->text()
          << sep << m_d->m_page->cleanIndentation->text()
          << sep << m_d->m_page->addFinalNewLine->text()
          << sep << m_d->m_page->encodingLabel->text()
          << sep << m_d->m_page->utf8BomLabel->text()
          << sep << m_d->m_page->mouseNavigation->text()
          << sep << m_d->m_page->scrollWheelZooming->text()
          << sep << m_d->m_page->groupBoxTabAndIndentSettings->title()
          << sep << m_d->m_page->groupBoxStorageSettings->title()
          << sep << m_d->m_page->groupBoxEncodings->title()
          << sep << m_d->m_page->groupBoxMouse->title();
        m_d->m_searchKeywords.remove(QLatin1Char('&'));
    }

    QSettings *settings = Core::ICore::instance()->settings();
    QTextCodec *defaultTextCodec = QTextCodec::codecForLocale();
    if (QTextCodec *candidate = QTextCodec::codecForName(
            settings->value(QLatin1String(Core::Constants::SETTINGS_DEFAULTTEXTENCODING)).toByteArray()))
        defaultTextCodec = candidate;
    QList<int> mibs = QTextCodec::availableMibs();
    qSort(mibs);
    QList<int> sortedMibs;
    foreach (int mib, mibs)
        if (mib >= 0)
            sortedMibs += mib;
    foreach (int mib, mibs)
        if (mib < 0)
            sortedMibs += mib;
    for (int i = 0; i < sortedMibs.count(); i++) {
        QTextCodec *codec = QTextCodec::codecForMib(sortedMibs.at(i));
        m_codecs += codec;
        QString name = codec->name();
        foreach (const QByteArray &alias, codec->aliases()) {
            name += QLatin1String(" / ");
            name += QString::fromLatin1(alias);
        }
        m_d->m_page->encodingBox->addItem(name);
        if (defaultTextCodec == codec)
            m_d->m_page->encodingBox->setCurrentIndex(i);
    }

    m_d->m_page->utf8BomBox->setCurrentIndex(Core::EditorManager::instance()->utf8BomSetting());

    return w;
}

void BehaviorSettingsPage::apply()
{
    if (!m_d->m_page) // page was never shown
        return;
    TabSettings newTabSettings;
    StorageSettings newStorageSettings;
    BehaviorSettings newBehaviorSettings;

    settingsFromUI(newTabSettings, newStorageSettings, newBehaviorSettings);

    Core::ICore *core = Core::ICore::instance();
    QSettings *s = core->settings();

    if (newTabSettings != m_d->m_tabSettings) {
        m_d->m_tabSettings = newTabSettings;
        if (s)
            m_d->m_tabSettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit tabSettingsChanged(newTabSettings);
    }

    if (newStorageSettings != m_d->m_storageSettings) {
        m_d->m_storageSettings = newStorageSettings;
        if (s)
            m_d->m_storageSettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit storageSettingsChanged(newStorageSettings);
    }

    if (newBehaviorSettings != m_d->m_behaviorSettings) {
        m_d->m_behaviorSettings = newBehaviorSettings;
        if (s)
            m_d->m_behaviorSettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit behaviorSettingsChanged(newBehaviorSettings);
    }

    QSettings* settings = Core::ICore::instance()->settings();
    settings->setValue(QLatin1String(Core::Constants::SETTINGS_DEFAULTTEXTENCODING),
                       m_codecs.at(m_d->m_page->encodingBox->currentIndex())->name());

    Core::EditorManager::instance()->setUtf8BomSetting(
                Core::IFile::Utf8BomSetting(m_d->m_page->utf8BomBox->currentIndex()));
}

void BehaviorSettingsPage::finish()
{
    if (!m_d->m_page) // page was never shown
        return;
    delete m_d->m_page;
    m_d->m_page = 0;
}

void BehaviorSettingsPage::settingsFromUI(TabSettings &tabSettings,
                                          StorageSettings &storageSettings,
                                          BehaviorSettings &behaviorSettings) const
{
    tabSettings.m_spacesForTabs = m_d->m_page->insertSpaces->isChecked();
    tabSettings.m_autoSpacesForTabs = m_d->m_page->autoInsertSpaces->isChecked();
    tabSettings.m_autoIndent = m_d->m_page->autoIndent->isChecked();
    tabSettings.m_smartBackspace = m_d->m_page->smartBackspace->isChecked();
    tabSettings.m_tabSize = m_d->m_page->tabSize->value();
    tabSettings.m_indentSize = m_d->m_page->indentSize->value();
    tabSettings.m_indentBraces = m_d->m_page->indentBlocksBehavior->currentIndex() >= 1;
    tabSettings.m_doubleIndentBlocks = m_d->m_page->indentBlocksBehavior->currentIndex() >= 2;

    tabSettings.m_tabKeyBehavior = (TabSettings::TabKeyBehavior)m_d->m_page->tabKeyBehavior->currentIndex();
    tabSettings.m_continuationAlignBehavior = (TabSettings::ContinuationAlignBehavior)m_d->m_page->continuationAlignBehavior->currentIndex();

    storageSettings.m_cleanWhitespace = m_d->m_page->cleanWhitespace->isChecked();
    storageSettings.m_inEntireDocument = m_d->m_page->inEntireDocument->isChecked();
    storageSettings.m_cleanIndentation = m_d->m_page->cleanIndentation->isChecked();
    storageSettings.m_addFinalNewLine = m_d->m_page->addFinalNewLine->isChecked();

    behaviorSettings.m_mouseNavigation = m_d->m_page->mouseNavigation->isChecked();
    behaviorSettings.m_scrollWheelZooming = m_d->m_page->scrollWheelZooming->isChecked();
}

void BehaviorSettingsPage::settingsToUI()
{
    const TabSettings &tabSettings = m_d->m_tabSettings;
    m_d->m_page->insertSpaces->setChecked(tabSettings.m_spacesForTabs);
    m_d->m_page->autoInsertSpaces->setChecked(tabSettings.m_autoSpacesForTabs);
    m_d->m_page->autoIndent->setChecked(tabSettings.m_autoIndent);
    m_d->m_page->smartBackspace->setChecked(tabSettings.m_smartBackspace);
    m_d->m_page->tabSize->setValue(tabSettings.m_tabSize);
    m_d->m_page->indentSize->setValue(tabSettings.m_indentSize);
    m_d->m_page->indentBlocksBehavior->setCurrentIndex(tabSettings.m_indentBraces ?
                                                      (tabSettings.m_doubleIndentBlocks ? 2 : 1)
                                                        : 0);
    m_d->m_page->tabKeyBehavior->setCurrentIndex(tabSettings.m_tabKeyBehavior);
    m_d->m_page->continuationAlignBehavior->setCurrentIndex(tabSettings.m_continuationAlignBehavior);

    const StorageSettings &storageSettings = m_d->m_storageSettings;
    m_d->m_page->cleanWhitespace->setChecked(storageSettings.m_cleanWhitespace);
    m_d->m_page->inEntireDocument->setChecked(storageSettings.m_inEntireDocument);
    m_d->m_page->cleanIndentation->setChecked(storageSettings.m_cleanIndentation);
    m_d->m_page->addFinalNewLine->setChecked(storageSettings.m_addFinalNewLine);

    const BehaviorSettings &behaviorSettings = m_d->m_behaviorSettings;
    m_d->m_page->mouseNavigation->setChecked(behaviorSettings.m_mouseNavigation);
    m_d->m_page->scrollWheelZooming->setChecked(behaviorSettings.m_scrollWheelZooming);
}

const TabSettings &BehaviorSettingsPage::tabSettings() const
{
    return m_d->m_tabSettings;
}

const StorageSettings &BehaviorSettingsPage::storageSettings() const
{
    return m_d->m_storageSettings;
}

const BehaviorSettings &BehaviorSettingsPage::behaviorSettings() const
{
    return m_d->m_behaviorSettings;
}

bool BehaviorSettingsPage::matches(const QString &s) const
{
    return m_d->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
