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

#ifndef SYMBOLPATHLISTEDITOR_H
#define SYMBOLPATHLISTEDITOR_H

#include <utils/pathlisteditor.h>

#include <QtGui/QDialog>

namespace Utils {
    class PathChooser;
}

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

// Internal helper dialog prompting for a cache directory
// using a PathChooser.
// Note that QFileDialog does not offer a way of suggesting
// a non-existent folder, which is in turn automatically
// created. This is done here (suggest $TEMP\symbolcache
// regardless of its existence).

class CacheDirectoryDialog    : public QDialog {
    Q_OBJECT
public:
   explicit CacheDirectoryDialog(QWidget *parent = 0);

   void setPath(const QString &p);
   QString path() const;

   virtual void accept();

private:
   Utils::PathChooser *m_chooser;
   QDialogButtonBox *m_buttonBox;
};

class CdbSymbolPathListEditor : public Utils::PathListEditor
{
    Q_OBJECT
public:
    explicit CdbSymbolPathListEditor(QWidget *parent = 0);

    static QString promptCacheDirectory(QWidget *parent);

    // Pre- and Postfix used to build a symbol server path specification
    static const char *symbolServerPrefixC;
    static const char *symbolServerPostfixC;

    // Format a symbol server specification with a local cache directory
    static QString symbolServerPath(const QString &cacheDir);
    // Check for a symbol server path and extract local cache directory
    static bool isSymbolServerPath(const QString &path, QString *cacheDir = 0);
    // Check for symbol server in list of paths.
    static int indexOfSymbolServerPath(const QStringList &paths, QString *cacheDir = 0);

    // Nag user to add a symbol server to the path list on debugger startup.
    static bool promptToAddSymbolServer(const QString &settingsGroup, QStringList *symbolPaths);

private slots:
    void addSymbolServer();
};

} // namespace Internal
} // namespace Debugger

#endif // SYMBOLPATHLISTEDITOR_H
