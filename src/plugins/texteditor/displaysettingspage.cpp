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

#include "displaysettingspage.h"
#include "displaysettings.h"
#include "ui_displaysettingspage.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtCore/QTextStream>

using namespace TextEditor;

struct DisplaySettingsPage::DisplaySettingsPagePrivate
{
    explicit DisplaySettingsPagePrivate(const DisplaySettingsPageParameters &p);

    const DisplaySettingsPageParameters m_parameters;
    Ui::DisplaySettingsPage *m_page;
    DisplaySettings m_displaySettings;
    QString m_searchKeywords;
};

DisplaySettingsPage::DisplaySettingsPagePrivate::DisplaySettingsPagePrivate
    (const DisplaySettingsPageParameters &p)
    : m_parameters(p), m_page(0)
{
    if (const QSettings *s = Core::ICore::instance()->settings()) {
        m_displaySettings.fromSettings(m_parameters.settingsPrefix, s);
    }
}

DisplaySettingsPage::DisplaySettingsPage(const DisplaySettingsPageParameters &p,
                                         QObject *parent)
  : TextEditorOptionsPage(parent),
    m_d(new DisplaySettingsPagePrivate(p))
{
}

DisplaySettingsPage::~DisplaySettingsPage()
{
    delete m_d;
}

QString DisplaySettingsPage::id() const
{
    return m_d->m_parameters.id;
}

QString DisplaySettingsPage::displayName() const
{
    return m_d->m_parameters.displayName;
}

QWidget *DisplaySettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->m_page = new Ui::DisplaySettingsPage;
    m_d->m_page->setupUi(w);
    settingsToUI();
    if (m_d->m_searchKeywords.isEmpty()) {
        QTextStream(&m_d->m_searchKeywords) << m_d->m_page->displayLineNumbers->text()
          << ' ' << m_d->m_page->highlightCurrentLine->text()
          << ' ' << m_d->m_page->displayFoldingMarkers->text()
          << ' ' << m_d->m_page->highlightBlocks->text()
          << ' ' << m_d->m_page->visualizeWhitespace->text()
          << ' ' << m_d->m_page->animateMatchingParentheses->text()
          << ' ' << m_d->m_page->enableTextWrapping->text()
          << ' ' << m_d->m_page->autoFoldFirstComment->text()
          << ' ' << m_d->m_page->centerOnScroll->text();
        m_d->m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

void DisplaySettingsPage::apply()
{
    if (!m_d->m_page) // page was never shown
        return;
    DisplaySettings newDisplaySettings;

    settingsFromUI(newDisplaySettings);
    setDisplaySettings(newDisplaySettings);
}

void DisplaySettingsPage::finish()
{
    if (!m_d->m_page) // page was never shown
        return;
    delete m_d->m_page;
    m_d->m_page = 0;
}

void DisplaySettingsPage::settingsFromUI(DisplaySettings &displaySettings) const
{
    displaySettings.m_displayLineNumbers = m_d->m_page->displayLineNumbers->isChecked();
    displaySettings.m_textWrapping = m_d->m_page->enableTextWrapping->isChecked();
    displaySettings.m_showWrapColumn = m_d->m_page->showWrapColumn->isChecked();
    displaySettings.m_wrapColumn = m_d->m_page->wrapColumn->value();
    displaySettings.m_visualizeWhitespace = m_d->m_page->visualizeWhitespace->isChecked();
    displaySettings.m_displayFoldingMarkers = m_d->m_page->displayFoldingMarkers->isChecked();
    displaySettings.m_highlightCurrentLine = m_d->m_page->highlightCurrentLine->isChecked();
    displaySettings.m_highlightBlocks = m_d->m_page->highlightBlocks->isChecked();
    displaySettings.m_animateMatchingParentheses = m_d->m_page->animateMatchingParentheses->isChecked();
    displaySettings.m_markTextChanges = m_d->m_page->markTextChanges->isChecked();
    displaySettings.m_autoFoldFirstComment = m_d->m_page->autoFoldFirstComment->isChecked();
    displaySettings.m_centerCursorOnScroll = m_d->m_page->centerOnScroll->isChecked();
}

void DisplaySettingsPage::settingsToUI()
{
    const DisplaySettings &displaySettings = m_d->m_displaySettings;
    m_d->m_page->displayLineNumbers->setChecked(displaySettings.m_displayLineNumbers);
    m_d->m_page->enableTextWrapping->setChecked(displaySettings.m_textWrapping);
    m_d->m_page->showWrapColumn->setChecked(displaySettings.m_showWrapColumn);
    m_d->m_page->wrapColumn->setValue(displaySettings.m_wrapColumn);
    m_d->m_page->visualizeWhitespace->setChecked(displaySettings.m_visualizeWhitespace);
    m_d->m_page->displayFoldingMarkers->setChecked(displaySettings.m_displayFoldingMarkers);
    m_d->m_page->highlightCurrentLine->setChecked(displaySettings.m_highlightCurrentLine);
    m_d->m_page->highlightBlocks->setChecked(displaySettings.m_highlightBlocks);
    m_d->m_page->animateMatchingParentheses->setChecked(displaySettings.m_animateMatchingParentheses);
    m_d->m_page->markTextChanges->setChecked(displaySettings.m_markTextChanges);
    m_d->m_page->autoFoldFirstComment->setChecked(displaySettings.m_autoFoldFirstComment);
    m_d->m_page->centerOnScroll->setChecked(displaySettings.m_centerCursorOnScroll);
}

const DisplaySettings &DisplaySettingsPage::displaySettings() const
{
    return m_d->m_displaySettings;
}

void DisplaySettingsPage::setDisplaySettings(const DisplaySettings &newDisplaySettings)
{
    if (newDisplaySettings != m_d->m_displaySettings) {
        m_d->m_displaySettings = newDisplaySettings;
        Core::ICore *core = Core::ICore::instance();
        if (QSettings *s = core->settings())
            m_d->m_displaySettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit displaySettingsChanged(newDisplaySettings);
    }
}

bool DisplaySettingsPage::matches(const QString &s) const
{
    return m_d->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}
