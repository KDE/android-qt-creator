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

#include "generalsettings.h"
#include "coreconstants.h"

#include <utils/stylehelper.h>
#include <utils/qtcolorbutton.h>
#include <utils/consoleprocess.h>
#include <utils/unixutils.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtCore/QLibraryInfo>
#include <QtCore/QSettings>

#include "ui_generalsettings.h"

using namespace Utils;
using namespace Core::Internal;


GeneralSettings::GeneralSettings():
    m_page(0), m_dialog(0)
{
}

QString GeneralSettings::id() const
{
    return QLatin1String(Core::Constants::SETTINGS_ID_ENVIRONMENT);
}

QString GeneralSettings::displayName() const
{
    return tr("General");
}

QString GeneralSettings::category() const
{
    return QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE);
}

QString GeneralSettings::displayCategory() const
{
    return QCoreApplication::translate("Core", Core::Constants::SETTINGS_TR_CATEGORY_CORE);
}

QIcon GeneralSettings::categoryIcon() const
{
    return QIcon(QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE_ICON));
}

static bool hasQmFilesForLocale(const QString &locale, const QString &creatorTrPath)
{
    static const QString qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    const QString trFile = QLatin1String("qt_") + locale + QLatin1String(".qm");
    return QFile::exists(qtTrPath+'/'+trFile) || QFile::exists(creatorTrPath+'/'+trFile);
}

void GeneralSettings::fillLanguageBox() const
{
    const QString currentLocale = language();

    m_page->languageBox->addItem(tr("<System Language>"), QString());
    // need to add this explicitly, since there is no qm file for English
    m_page->languageBox->addItem(QLatin1String("English"), QLatin1String("C"));
    if (currentLocale == QLatin1String("C"))
        m_page->languageBox->setCurrentIndex(m_page->languageBox->count() - 1);

    const QString creatorTrPath =
            Core::ICore::instance()->resourcePath() + QLatin1String("/translations");
    const QStringList languageFiles = QDir(creatorTrPath).entryList(QStringList(QLatin1String("qtcreator*.qm")));

    Q_FOREACH(const QString &languageFile, languageFiles)
    {
        int start = languageFile.indexOf(QLatin1Char('_'))+1;
        int end = languageFile.lastIndexOf(QLatin1Char('.'));
        const QString locale = languageFile.mid(start, end-start);
        // no need to show a language that creator will not load anyway
        if (hasQmFilesForLocale(locale, creatorTrPath)) {
            m_page->languageBox->addItem(QLocale::languageToString(QLocale(locale).language()), locale);
            if (locale == currentLocale)
                m_page->languageBox->setCurrentIndex(m_page->languageBox->count() - 1);
        }
    }
}

QWidget *GeneralSettings::createPage(QWidget *parent)
{
    m_page = new Ui::GeneralSettings();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    QSettings* settings = Core::ICore::instance()->settings();
    Q_UNUSED(settings) // Windows

    fillLanguageBox();

    m_page->colorButton->setColor(StyleHelper::requestedBaseColor());
    m_page->externalEditorEdit->setText(EditorManager::instance()->externalEditor());
    m_page->reloadBehavior->setCurrentIndex(EditorManager::instance()->reloadSetting());
#ifdef Q_OS_UNIX
    m_page->terminalEdit->setText(ConsoleProcess::terminalEmulator(settings));
#else
    m_page->terminalLabel->hide();
    m_page->terminalEdit->hide();
    m_page->resetTerminalButton->hide();
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    m_page->externalFileBrowserEdit->setText(UnixUtils::fileBrowser(settings));
#else
    m_page->externalFileBrowserLabel->hide();
    m_page->externalFileBrowserEdit->hide();
    m_page->resetFileBrowserButton->hide();
    m_page->helpExternalFileBrowserButton->hide();
#endif

    connect(m_page->resetButton, SIGNAL(clicked()),
            this, SLOT(resetInterfaceColor()));
    connect(m_page->resetEditorButton, SIGNAL(clicked()),
            this, SLOT(resetExternalEditor()));
    connect(m_page->helpExternalEditorButton, SIGNAL(clicked()),
            this, SLOT(showHelpForExternalEditor()));
#ifdef Q_OS_UNIX
    connect(m_page->resetTerminalButton, SIGNAL(clicked()),
            this, SLOT(resetTerminal()));
#ifndef Q_OS_MAC
    connect(m_page->resetFileBrowserButton, SIGNAL(clicked()),
            this, SLOT(resetFileBrowser()));
    connect(m_page->helpExternalFileBrowserButton, SIGNAL(clicked()),
            this, SLOT(showHelpForFileBrowser()));
#endif
#endif

    if (m_searchKeywords.isEmpty()) {
        QLatin1Char sep(' ');
        QTextStream(&m_searchKeywords)
                << m_page->interfaceBox->title() << sep
                << m_page->colorLabel->text() << sep
                << m_page->languageLabel->text() << sep
                << m_page->systemBox->title() << sep
                << m_page->terminalLabel->text() << sep
                << m_page->editorLabel->text() << sep
                << m_page->modifiedLabel->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

bool GeneralSettings::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void GeneralSettings::apply()
{
    if (!m_page) // wasn't shown, can't be changed
        return;
    int currentIndex = m_page->languageBox->currentIndex();
    setLanguage(m_page->languageBox->itemData(currentIndex, Qt::UserRole).toString());
    // Apply the new base color if accepted
    StyleHelper::setBaseColor(m_page->colorButton->color());
    EditorManager::instance()->setExternalEditor(m_page->externalEditorEdit->text());
    EditorManager::instance()->setReloadSetting(IFile::ReloadSetting(m_page->reloadBehavior->currentIndex()));
#ifdef Q_OS_UNIX
    ConsoleProcess::setTerminalEmulator(Core::ICore::instance()->settings(),
                                        m_page->terminalEdit->text());
#ifndef Q_OS_MAC
    Utils::UnixUtils::setFileBrowser(Core::ICore::instance()->settings(), m_page->externalFileBrowserEdit->text());
#endif
#endif
}

void GeneralSettings::finish()
{
    if (!m_page) // page was never shown
        return;
    delete m_page;
    m_page = 0;
}

void GeneralSettings::resetInterfaceColor()
{
    m_page->colorButton->setColor(StyleHelper::DEFAULT_BASE_COLOR);
}

void GeneralSettings::resetExternalEditor()
{
    m_page->externalEditorEdit->setText(EditorManager::instance()->defaultExternalEditor());
}

#ifdef Q_OS_UNIX
void GeneralSettings::resetTerminal()
{
    m_page->terminalEdit->setText(ConsoleProcess::defaultTerminalEmulator() + QLatin1String(" -e"));
}

#ifndef Q_OS_MAC
void GeneralSettings::resetFileBrowser()
{
    m_page->externalFileBrowserEdit->setText(UnixUtils::defaultFileBrowser());
}
#endif
#endif


void GeneralSettings::variableHelpDialogCreator(const QString &helpText)
{
    if (m_dialog) {
        if (m_dialog->text() != helpText)
            m_dialog->setText(helpText);

        m_dialog->show();
        m_dialog->raise();
        m_dialog->activateWindow();
        return;
    }
    QMessageBox *mb = new QMessageBox(QMessageBox::Information,
                                  tr("Variables"),
                                  helpText,
                                  QMessageBox::Close,
                                  m_page->helpExternalEditorButton);
    mb->setWindowModality(Qt::NonModal);
    m_dialog = mb;
    mb->show();
}


void GeneralSettings::showHelpForExternalEditor()
{
    variableHelpDialogCreator(EditorManager::instance()->externalEditorHelpText());
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
void GeneralSettings::showHelpForFileBrowser()
{
    variableHelpDialogCreator(UnixUtils::fileBrowserHelpText());
}
#endif

void GeneralSettings::resetLanguage()
{
    // system language is default
    m_page->languageBox->setCurrentIndex(0);
}

QString GeneralSettings::language() const
{
    QSettings* settings = Core::ICore::instance()->settings();
    return settings->value(QLatin1String("General/OverrideLanguage")).toString();
}

void GeneralSettings::setLanguage(const QString &locale)
{
    QSettings* settings = Core::ICore::instance()->settings();
    if (settings->value(QLatin1String("General/OverrideLanguage")).toString() != locale)
    {
        QMessageBox::information(Core::ICore::instance()->mainWindow(), tr("Restart required"),
                                 tr("The language change will take effect after a restart of Qt Creator."));
    }
    if (locale.isEmpty())
        settings->remove(QLatin1String("General/OverrideLanguage"));
    else
        settings->setValue(QLatin1String("General/OverrideLanguage"), locale);
}
