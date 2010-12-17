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

#include "cppfilesettingspage.h"
#include "cpptoolsconstants.h"
#include "ui_cppfilesettingspage.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/mimedatabase.h>
#include <cppeditor/cppeditorconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/QDate>
#include <QtCore/QLocale>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

static const char *headerSuffixKeyC = "HeaderSuffix";
static const char *sourceSuffixKeyC = "SourceSuffix";
static const char *licenseTemplatePathKeyC = "LicenseTemplate";

const char *licenseTemplateTemplate = QT_TRANSLATE_NOOP("CppTools::Internal::CppFileSettingsWidget",
"/**************************************************************************\n"
"** Qt Creator license header template\n"
"**   Special keywords: %USER% %DATE% %YEAR%\n"
"**   Environment variables: %$VARIABLE%\n"
"**   To protect a percent sign, use '%%'.\n"
"**************************************************************************/\n");

namespace CppTools {
namespace Internal {

CppFileSettings::CppFileSettings() :
    lowerCaseFiles(false)
{
}

void CppFileSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    s->setValue(QLatin1String(headerSuffixKeyC), headerSuffix);
    s->setValue(QLatin1String(sourceSuffixKeyC), sourceSuffix);
    s->setValue(QLatin1String(Constants::LOWERCASE_CPPFILES_KEY), lowerCaseFiles);
    s->setValue(QLatin1String(licenseTemplatePathKeyC), licenseTemplatePath);
    s->endGroup();
}

void CppFileSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    headerSuffix= s->value(QLatin1String(headerSuffixKeyC), QLatin1String("h")).toString();
    sourceSuffix = s->value(QLatin1String(sourceSuffixKeyC), QLatin1String("cpp")).toString();
    const bool lowerCaseDefault = Constants::lowerCaseFilesDefault;
    lowerCaseFiles = s->value(QLatin1String(Constants::LOWERCASE_CPPFILES_KEY), QVariant(lowerCaseDefault)).toBool();
    licenseTemplatePath = s->value(QLatin1String(licenseTemplatePathKeyC), QString()).toString();
    s->endGroup();
}

bool CppFileSettings::applySuffixesToMimeDB()
{
    Core::MimeDatabase *mdb = Core::ICore::instance()->mimeDatabase();
    return mdb->setPreferredSuffix(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE), sourceSuffix)
            && mdb->setPreferredSuffix(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE), headerSuffix);
}

bool CppFileSettings::equals(const CppFileSettings &rhs) const
{
    return lowerCaseFiles == rhs.lowerCaseFiles
           && headerSuffix == rhs.headerSuffix
           && sourceSuffix == rhs.sourceSuffix
           && licenseTemplatePath == rhs.licenseTemplatePath;
}

// Replacements of special license template keywords.
static bool keyWordReplacement(const QString &keyWord,
                               const QString &file,
                               const QString &className,
                               QString *value)
{
    if (keyWord == QLatin1String("%YEAR%")) {
        *value = QString::number(QDate::currentDate().year());
        return true;
    }
    if (keyWord == QLatin1String("%MONTH%")) {
        *value = QString::number(QDate::currentDate().month());
        return true;
    }
    if (keyWord == QLatin1String("%DAY%")) {
        *value = QString::number(QDate::currentDate().day());
        return true;
    }
    if (keyWord == QLatin1String("%CLASS%")) {
        *value = className;
        return true;
    }
    if (keyWord == QLatin1String("%FILENAME%")) {
        *value = QFileInfo(file).fileName();
        return true;
    }
    if (keyWord == QLatin1String("%DATE%")) {
        static QString format;
        // ensure a format with 4 year digits. Some have locales have 2.
        if (format.isEmpty()) {
            QLocale loc;
            format = loc.dateFormat(QLocale::ShortFormat);
            const QChar ypsilon = QLatin1Char('y');
            if (format.count(ypsilon) == 2)
                format.insert(format.indexOf(ypsilon), QString(2, ypsilon));
        }
        *value = QDate::currentDate().toString(format);
        return true;
    }
    if (keyWord == QLatin1String("%USER%")) {
#ifdef Q_OS_WIN
        *value = QString::fromLocal8Bit(qgetenv("USERNAME"));
#else
        *value = QString::fromLocal8Bit(qgetenv("USER"));
#endif
        return true;
    }
    // Environment variables (for example '%$EMAIL%').
    if (keyWord.startsWith(QLatin1String("%$"))) {
        const QString varName = keyWord.mid(2, keyWord.size() - 3);
        *value = QString::fromLocal8Bit(qgetenv(varName.toLocal8Bit()));
        return true;
    }
    return false;
}

// Parse a license template, scan for %KEYWORD% and replace if known.
// Replace '%%' by '%'.
static void parseLicenseTemplatePlaceholders(QString *t, const QString &file, const QString &className)
{
    int pos = 0;
    const QChar placeHolder = QLatin1Char('%');
    do {
        const int placeHolderPos = t->indexOf(placeHolder, pos);
        if (placeHolderPos == -1)
            break;
        const int endPlaceHolderPos = t->indexOf(placeHolder, placeHolderPos + 1);
        if (endPlaceHolderPos == -1)
            break;
        if (endPlaceHolderPos == placeHolderPos + 1) { // '%%' -> '%'
            t->remove(placeHolderPos, 1);
            pos = placeHolderPos + 1;
        } else {
            const QString keyWord = t->mid(placeHolderPos, endPlaceHolderPos + 1 - placeHolderPos);
            QString replacement;
            if (keyWordReplacement(keyWord, file, className, &replacement)) {
                t->replace(placeHolderPos, keyWord.size(), replacement);
                pos = placeHolderPos + replacement.size();
            } else {
                // Leave invalid keywords as is.
                pos = endPlaceHolderPos + 1;
            }
        }
    } while (pos < t->size());
}

// Convenience that returns the formatted license template.
QString CppFileSettings::licenseTemplate(const QString &fileName, const QString &className)
{

    const QSettings *s = Core::ICore::instance()->settings();
    QString key = QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP);
    key += QLatin1Char('/');
    key += QLatin1String(licenseTemplatePathKeyC);
    const QString path = s->value(key, QString()).toString();
    if (path.isEmpty())
        return QString();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qWarning("Unable to open the license template %s: %s", qPrintable(path), qPrintable(file.errorString()));
        return QString();
    }

    QTextCodec *codec = Core::EditorManager::instance()->defaultTextEncoding();
    QTextStream licenseStream(&file);
    licenseStream.setCodec(codec);
    licenseStream.setAutoDetectUnicode(true);
    QString license = licenseStream.readAll();

    parseLicenseTemplatePlaceholders(&license, fileName, className);
    // Ensure exactly one additional new line separating stuff
    const QChar newLine = QLatin1Char('\n');
    if (!license.endsWith(newLine))
        license += newLine;
    license += newLine;
    return license;
}

// ------------------ CppFileSettingsWidget

CppFileSettingsWidget::CppFileSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::CppFileSettingsPage)
{
    m_ui->setupUi(this);
    const Core::MimeDatabase *mdb = Core::ICore::instance()->mimeDatabase();
    // populate suffix combos
    if (const Core::MimeType sourceMt = mdb->findByType(QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)))
        foreach (const QString &suffix, sourceMt.suffixes())
            m_ui->sourceSuffixComboBox->addItem(suffix);

    if (const Core::MimeType headerMt = mdb->findByType(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)))
        foreach (const QString &suffix, headerMt.suffixes())
            m_ui->headerSuffixComboBox->addItem(suffix);
    m_ui->licenseTemplatePathChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->licenseTemplatePathChooser->addButton(tr("Edit..."), this, SLOT(slotEdit()));
}

CppFileSettingsWidget::~CppFileSettingsWidget()
{
    delete m_ui;
}

QString CppFileSettingsWidget::licenseTemplatePath() const
{
    return m_ui->licenseTemplatePathChooser->path();
}

void CppFileSettingsWidget::setLicenseTemplatePath(const QString &lp)
{
    m_ui->licenseTemplatePathChooser->setPath(lp);
}

CppFileSettings CppFileSettingsWidget::settings() const
{
    CppFileSettings rc;
    rc.lowerCaseFiles = m_ui->lowerCaseFileNamesCheckBox->isChecked();
    rc.headerSuffix = m_ui->headerSuffixComboBox->currentText();
    rc.sourceSuffix = m_ui->sourceSuffixComboBox->currentText();
    rc.licenseTemplatePath = licenseTemplatePath();
    return rc;
}

QString CppFileSettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui->headerSuffixLabel->text()
            << ' ' << m_ui->sourceSuffixLabel->text()
            << ' ' << m_ui->lowerCaseFileNamesCheckBox->text()
            << ' ' << m_ui->licenseTemplateLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

static inline void setComboText(QComboBox *cb, const QString &text, int defaultIndex = 0)
{
    const int index = cb->findText(text);
    cb->setCurrentIndex(index == -1 ? defaultIndex: index);
}

void CppFileSettingsWidget::setSettings(const CppFileSettings &s)
{
    m_ui->lowerCaseFileNamesCheckBox->setChecked(s.lowerCaseFiles);
    setComboText(m_ui->headerSuffixComboBox, s.headerSuffix);
    setComboText(m_ui->sourceSuffixComboBox, s.sourceSuffix);
    setLicenseTemplatePath(s.licenseTemplatePath);
}

void CppFileSettingsWidget::slotEdit()
{
    QString path = licenseTemplatePath();
    // Edit existing file with C++
    if (!path.isEmpty()) {
        Core::EditorManager::instance()->openEditor(path, QLatin1String(CppEditor::Constants::CPPEDITOR_ID),
                                                    Core::EditorManager::ModeSwitch);
        return;
    }
    // Pick a file name and write new template, edit with C++
    path = QFileDialog::getSaveFileName(this, tr("Choose Location for New License Template File"));
    if (path.isEmpty())
        return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text|QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("Template write error"),
                             tr("Cannot write to %1: %2").arg(path, file.errorString()));
        return;
    }
    file.write(tr(licenseTemplateTemplate).toUtf8());
    file.close();
    setLicenseTemplatePath(path);
    Core::EditorManager::instance()->openEditor(path, QLatin1String(CppEditor::Constants::CPPEDITOR_ID),
                                                Core::EditorManager::ModeSwitch);
}

// --------------- CppFileSettingsPage
CppFileSettingsPage::CppFileSettingsPage(QSharedPointer<CppFileSettings> &settings,
                                         QObject *parent) :
    Core::IOptionsPage(parent),
    m_settings(settings)
{
}

CppFileSettingsPage::~CppFileSettingsPage()
{
}

QString CppFileSettingsPage::id() const
{
    return QLatin1String(Constants::CPP_SETTINGS_ID);
}

QString CppFileSettingsPage::displayName() const
{
    return QCoreApplication::translate("CppTools", Constants::CPP_SETTINGS_NAME);
}

QString CppFileSettingsPage::category() const
{
    return QLatin1String(Constants::CPP_SETTINGS_CATEGORY);
}

QString CppFileSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("CppTools", Constants::CPP_SETTINGS_TR_CATEGORY);
}

QIcon CppFileSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(Constants::SETTINGS_CATEGORY_CPP_ICON));
}

QWidget *CppFileSettingsPage::createPage(QWidget *parent)
{

    m_widget = new CppFileSettingsWidget(parent);
    m_widget->setSettings(*m_settings);
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void CppFileSettingsPage::apply()
{
    if (m_widget) {
        const CppFileSettings newSettings = m_widget->settings();
        if (newSettings != *m_settings) {
            *m_settings = newSettings;
            m_settings->toSettings(Core::ICore::instance()->settings());
            m_settings->applySuffixesToMimeDB();
        }
    }
}

bool CppFileSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

} // namespace Internal
} // namespace CppTools
