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

#include "cdbsymbolpathlisteditor.h"

#include <coreplugin/icore.h>

#include <utils/pathchooser.h>
#include <utils/checkablemessagebox.h>

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtGui/QFileDialog>
#include <QtGui/QAction>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

namespace Debugger {
namespace Internal {

CacheDirectoryDialog::CacheDirectoryDialog(QWidget *parent) :
    QDialog(parent), m_chooser(new Utils::PathChooser),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel))
{
    setWindowTitle(tr("Select Local Cache Folder"));
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QFormLayout *formLayout = new QFormLayout;
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_chooser->setMinimumWidth(400);
    formLayout->addRow(tr("Path:"), m_chooser);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_buttonBox);

    setLayout(mainLayout);

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void CacheDirectoryDialog::setPath(const QString &p)
{
    m_chooser->setPath(p);
}

QString CacheDirectoryDialog::path() const
{
    return m_chooser->path();
}

void CacheDirectoryDialog::accept()
{
    // Ensure path exists
    QString cache = path();
    if (cache.isEmpty())
        return;
    QFileInfo fi(cache);
    // Folder exists - all happy.
    if (fi.isDir()) {
        QDialog::accept();
        return;
    }
    // Does a file of the same name exist?
    if (fi.exists()) {
        QMessageBox::warning(this, tr("Already Exists"),
                             tr("A file named '%1' already exists.").arg(cache));
        return;
    }
    // Create
    QDir root(QDir::root());
    if (!root.mkpath(cache)) {
        QMessageBox::warning(this, tr("Cannot Create"),
                             tr("The folder '%1' could not be created.").arg(cache));
        return;
    }
    QDialog::accept();
}

// ---------------- CdbSymbolPathListEditor

const char *CdbSymbolPathListEditor::symbolServerPrefixC = "symsrv*symsrv.dll*";
const char *CdbSymbolPathListEditor::symbolServerPostfixC = "*http://msdl.microsoft.com/download/symbols";

CdbSymbolPathListEditor::CdbSymbolPathListEditor(QWidget *parent) :
    Utils::PathListEditor(parent)
{
    //! Add Microsoft Symbol server connection
    QAction *action = insertAction(lastAddActionIndex() + 1, tr("Symbol Server..."), this, SLOT(addSymbolServer()));
    action->setToolTip(tr("Adds the Microsoft symbol server providing symbols for operating system libraries."
                      "Requires specifying a local cache directory."));
}

QString CdbSymbolPathListEditor::promptCacheDirectory(QWidget *parent)
{
    CacheDirectoryDialog dialog(parent);
    dialog.setPath(QDir::tempPath() + QDir::separator() + QLatin1String("symbolcache"));
    if (dialog.exec() != QDialog::Accepted)
        return QString();
    return dialog.path();
}

void CdbSymbolPathListEditor::addSymbolServer()
{
    const QString cacheDir = promptCacheDirectory(this);
    if (!cacheDir.isEmpty())
        insertPathAtCursor(CdbSymbolPathListEditor::symbolServerPath(cacheDir));
}

QString CdbSymbolPathListEditor::symbolServerPath(const QString &cacheDir)
{
    QString s = QLatin1String(symbolServerPrefixC);
    s +=  QDir::toNativeSeparators(cacheDir);
    s += QLatin1String(symbolServerPostfixC);
    return s;
}

bool CdbSymbolPathListEditor::isSymbolServerPath(const QString &path, QString *cacheDir /*  = 0 */)
{
    // Split apart symbol server post/prefixes
    if (!path.startsWith(QLatin1String(symbolServerPrefixC)) || !path.endsWith(QLatin1String(symbolServerPostfixC)))
        return false;
    if (cacheDir) {
        const unsigned prefixLength = qstrlen(symbolServerPrefixC);
        *cacheDir = path.mid(prefixLength, path.size() - prefixLength - qstrlen(symbolServerPostfixC));
    }
    return true;
}

int CdbSymbolPathListEditor::indexOfSymbolServerPath(const QStringList &paths, QString *cacheDir /*  = 0 */)
{
    const int count = paths.size();
    for (int i = 0; i < count; i++)
        if (CdbSymbolPathListEditor::isSymbolServerPath(paths.at(i), cacheDir))
            return i;
    return -1;
}

bool CdbSymbolPathListEditor::promptToAddSymbolServer(const QString &settingsGroup, QStringList *symbolPaths)
{
    // Check symbol server unless the user has an external/internal setup
    if (!qgetenv("_NT_SYMBOL_PATH").isEmpty()
            || CdbSymbolPathListEditor::indexOfSymbolServerPath(*symbolPaths) != -1)
        return false;
    // Prompt to use Symbol server unless the user checked "No nagging".
    Core::ICore *core = Core::ICore::instance();
    const QString nagSymbolServerKey = settingsGroup + QLatin1String("/NoPromptSymbolServer");
    bool noFurtherNagging = core->settings()->value(nagSymbolServerKey, false).toBool();
    if (noFurtherNagging)
        return false;

    const QString symServUrl = QLatin1String("http://support.microsoft.com/kb/311503");
    const QString msg = tr("<html><head/><body><p>The debugger is not configured to use the public "
                           "<a href=\"%1\">Microsoft Symbol Server</a>. This is recommended "
                           "for retrieval of the symbols of the operating system libraries.</p>"
                           "<p><i>Note:</i> A fast internet connection is required for this to work smoothly. Also, a delay "
                           "might occur when connecting for the first time.</p>"
                           "<p>Would you like to set it up?</p></br>"
                           "</body></html>").arg(symServUrl);
    const QDialogButtonBox::StandardButton answer =
            Utils::CheckableMessageBox::question(core->mainWindow(), tr("Symbol Server"), msg,
                                                 tr("Do not ask again"), &noFurtherNagging);
    core->settings()->setValue(nagSymbolServerKey, noFurtherNagging);
    if (answer == QDialogButtonBox::No)
        return false;
    // Prompt for path and add it. Synchronize QSetting and debugger.
    const QString cacheDir = CdbSymbolPathListEditor::promptCacheDirectory(core->mainWindow());
    if (cacheDir.isEmpty())
        return false;

    symbolPaths->push_back(CdbSymbolPathListEditor::symbolServerPath(cacheDir));
    return true;
}

} // namespace Internal
} // namespace Debugger
