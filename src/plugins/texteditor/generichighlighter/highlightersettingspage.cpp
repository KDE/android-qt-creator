/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "highlightersettingspage.h"
#include "highlightersettings.h"
#include "manager.h"
#include "managedefinitionsdialog.h"
#include "ui_highlightersettingspage.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>

#include <QtGui/QMessageBox>

using namespace TextEditor;
using namespace Internal;

struct HighlighterSettingsPage::HighlighterSettingsPagePrivate
{
    explicit HighlighterSettingsPagePrivate(const QString &id);

    const QString m_id;
    const QString m_displayName;
    const QString m_settingsPrefix;

    QString m_searchKeywords;

    HighlighterSettings m_settings;

    Ui::HighlighterSettingsPage *m_page;
};

HighlighterSettingsPage::HighlighterSettingsPagePrivate::
HighlighterSettingsPagePrivate(const QString &id) :
    m_id(id),
    m_displayName(tr("Generic Highlighter")),
    m_settingsPrefix(QLatin1String("Text")),
    m_page(0)
{}

HighlighterSettingsPage::HighlighterSettingsPage(const QString &id, QObject *parent) :
    TextEditorOptionsPage(parent),
    m_requestMimeTypeRegistration(false),
    m_d(new HighlighterSettingsPagePrivate(id))
{
    if (QSettings *s = Core::ICore::instance()->settings())
        m_d->m_settings.fromSettings(m_d->m_settingsPrefix, s);
}

HighlighterSettingsPage::~HighlighterSettingsPage()
{
    delete m_d;
}

QString HighlighterSettingsPage::id() const
{
    return m_d->m_id;
}

QString HighlighterSettingsPage::displayName() const
{
    return m_d->m_displayName;
}

QWidget *HighlighterSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->m_page = new Ui::HighlighterSettingsPage;
    m_d->m_page->setupUi(w);
    m_d->m_page->definitionFilesPath->setExpectedKind(Utils::PathChooser::Directory);
    m_d->m_page->definitionFilesPath->addButton(tr("Download Definitions"), this,
                                               SLOT(requestAvailableDefinitionsMetaData()));
    m_d->m_page->fallbackDefinitionFilesPath->setExpectedKind(Utils::PathChooser::Directory);
    m_d->m_page->fallbackDefinitionFilesPath->addButton(tr("Autodetect"), this,
                                                       SLOT(resetDefinitionsLocation()));

    settingsToUI();

    if (m_d->m_searchKeywords.isEmpty()) {
        QTextStream(&m_d->m_searchKeywords) << m_d->m_page->definitionFilesGroupBox->title()
            << m_d->m_page->locationLabel->text()
            << m_d->m_page->alertWhenNoDefinition->text()
            << m_d->m_page->useFallbackLocation->text()
            << m_d->m_page->ignoreLabel->text();
    }

    connect(m_d->m_page->useFallbackLocation, SIGNAL(clicked(bool)),
            this, SLOT(setFallbackLocationState(bool)));
    connect(m_d->m_page->definitionFilesPath, SIGNAL(validChanged(bool)),
            this, SLOT(setDownloadDefinitionsState(bool)));
    connect(w, SIGNAL(destroyed()), this, SLOT(ignoreDownloadReply()));

    return w;
}

void HighlighterSettingsPage::apply()
{
    if (!m_d->m_page) // page was not shown
        return;
    if (settingsChanged())
        settingsFromUI();

    if (m_requestMimeTypeRegistration) {
        Manager::instance()->registerMimeTypes();
        m_requestMimeTypeRegistration = false;
    }
}

void HighlighterSettingsPage::finish()
{
    if (!m_d->m_page) // page was not shown
        return;
    delete m_d->m_page;
    m_d->m_page = 0;
}

bool HighlighterSettingsPage::matches(const QString &s) const
{
    return m_d->m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

const HighlighterSettings &HighlighterSettingsPage::highlighterSettings() const
{
    return m_d->m_settings;
}

void HighlighterSettingsPage::settingsFromUI()
{
    if (!m_requestMimeTypeRegistration && (
        m_d->m_settings.definitionFilesPath() != m_d->m_page->definitionFilesPath->path() ||
        m_d->m_settings.fallbackDefinitionFilesPath() !=
            m_d->m_page->fallbackDefinitionFilesPath->path() ||
        m_d->m_settings.useFallbackLocation() != m_d->m_page->useFallbackLocation->isChecked())) {
        m_requestMimeTypeRegistration = true;
    }

    m_d->m_settings.setDefinitionFilesPath(m_d->m_page->definitionFilesPath->path());
    m_d->m_settings.setFallbackDefinitionFilesPath(m_d->m_page->fallbackDefinitionFilesPath->path());
    m_d->m_settings.setAlertWhenNoDefinition(m_d->m_page->alertWhenNoDefinition->isChecked());
    m_d->m_settings.setUseFallbackLocation(m_d->m_page->useFallbackLocation->isChecked());
    m_d->m_settings.setIgnoredFilesPatterns(m_d->m_page->ignoreEdit->text());
    if (QSettings *s = Core::ICore::instance()->settings())
        m_d->m_settings.toSettings(m_d->m_settingsPrefix, s);
}

void HighlighterSettingsPage::settingsToUI()
{
    m_d->m_page->definitionFilesPath->setPath(m_d->m_settings.definitionFilesPath());
    m_d->m_page->fallbackDefinitionFilesPath->setPath(m_d->m_settings.fallbackDefinitionFilesPath());
    m_d->m_page->alertWhenNoDefinition->setChecked(m_d->m_settings.alertWhenNoDefinition());
    m_d->m_page->useFallbackLocation->setChecked(m_d->m_settings.useFallbackLocation());
    m_d->m_page->ignoreEdit->setText(m_d->m_settings.ignoredFilesPatterns());

    setFallbackLocationState(m_d->m_page->useFallbackLocation->isChecked());
    setDownloadDefinitionsState(m_d->m_page->definitionFilesPath->isValid());
}

void HighlighterSettingsPage::resetDefinitionsLocation()
{
    const QString &location = findFallbackDefinitionsLocation();
    if (location.isEmpty())
        QMessageBox::information(0, tr("Autodetect Definitions"),
                                 tr("No pre-installed definitions could be found."));
    else
        m_d->m_page->fallbackDefinitionFilesPath->setPath(location);
}

void HighlighterSettingsPage::requestAvailableDefinitionsMetaData()
{
    setDownloadDefinitionsState(false);

    connect(Manager::instance(),
            SIGNAL(definitionsMetaDataReady(QList<Internal::HighlightDefinitionMetaData>)),
            this,
            SLOT(manageDefinitions(QList<Internal::HighlightDefinitionMetaData>)),
            Qt::UniqueConnection);
    connect(Manager::instance(), SIGNAL(errorDownloadingDefinitionsMetaData()),
            this, SLOT(showError()), Qt::UniqueConnection);
    Manager::instance()->downloadAvailableDefinitionsMetaData();
}

void HighlighterSettingsPage::ignoreDownloadReply()
{
    disconnect(Manager::instance(),
               SIGNAL(definitionsMetaDataReady(QList<Internal::HighlightDefinitionMetaData>)),
               this,
               SLOT(manageDefinitions(QList<Internal::HighlightDefinitionMetaData>)));
    disconnect(Manager::instance(), SIGNAL(errorDownloadingDefinitionsMetaData()),
               this, SLOT(showError()));
}

void HighlighterSettingsPage::manageDefinitions(const QList<HighlightDefinitionMetaData> &metaData)
{
    ManageDefinitionsDialog dialog(metaData,
                                   m_d->m_page->definitionFilesPath->path() + QLatin1Char('/'),
                                   m_d->m_page->definitionFilesPath->buttonAtIndex(1)->window());
    if (dialog.exec() && !m_requestMimeTypeRegistration)
        m_requestMimeTypeRegistration = true;
    setDownloadDefinitionsState(m_d->m_page->definitionFilesPath->isValid());
}

void HighlighterSettingsPage::showError()
{
    QMessageBox::critical(m_d->m_page->definitionFilesPath->buttonAtIndex(1)->window(),
                          tr("Error connecting to server."),
                          tr("Not possible to retrieve data."));
    setDownloadDefinitionsState(m_d->m_page->definitionFilesPath->isValid());
}

void HighlighterSettingsPage::setFallbackLocationState(bool checked)
{
    m_d->m_page->fallbackDefinitionFilesPath->setEnabled(checked);
}

void HighlighterSettingsPage::setDownloadDefinitionsState(bool valid)
{
    m_d->m_page->definitionFilesPath->buttonAtIndex(1)->setEnabled(valid);
}

bool HighlighterSettingsPage::settingsChanged() const
{
    if (m_d->m_settings.definitionFilesPath() != m_d->m_page->definitionFilesPath->path())
        return true;
    if (m_d->m_settings.fallbackDefinitionFilesPath() !=
            m_d->m_page->fallbackDefinitionFilesPath->path())
        return true;
    if (m_d->m_settings.alertWhenNoDefinition() != m_d->m_page->alertWhenNoDefinition->isChecked())
        return true;
    if (m_d->m_settings.useFallbackLocation() != m_d->m_page->useFallbackLocation->isChecked())
        return true;
    if (m_d->m_settings.ignoredFilesPatterns() != m_d->m_page->ignoreEdit->text())
        return true;
    return false;
}
