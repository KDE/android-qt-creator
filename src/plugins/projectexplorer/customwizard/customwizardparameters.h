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

#ifndef CUSTOMPROJECTWIZARDPARAMETERS_H
#define CUSTOMPROJECTWIZARDPARAMETERS_H

#include <coreplugin/basefilewizard.h>

#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QIODevice;
class QDebug;
class QTemporaryFile;
class QScriptEngine;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

struct CustomWizardField {
    // Parameters of the widget control are stored as map
    typedef QMap<QString, QString> ControlAttributeMap;
    CustomWizardField();
    void clear();

    // Attribute map keys for combo entries
    static QString comboEntryValueKey(int i);
    static QString comboEntryTextKey(int i);

    QString description;
    QString name;
    ControlAttributeMap controlAttributes;
    bool mandatory;
};

struct CustomWizardFile {
    CustomWizardFile();

    QString source;
    QString target;
    bool openEditor;
    bool openProject;
    bool binary;
};

// Documentation inside.
struct CustomWizardValidationRule {
    // Validate a set of rules and return false + message on the first failing one.
    static bool validateRules(const QList<CustomWizardValidationRule> &rules,
                              const QMap<QString, QString> &replacementMap,
                              QString *errorMessage);
    bool validate(QScriptEngine &, const QMap<QString, QString> &replacementMap) const;
    QString condition;
    QString message;
};

// Documentation inside.
struct GeneratorScriptArgument {
    enum Flags {
        // Omit this arguments if all field placeholders expanded to empty strings.
        OmitEmpty = 0x1,
        // Do use the actual field value, but write it to a temporary
        // text file and inserts its file name (suitable for multiline texts).
        WriteFile = 0x2 };

    explicit GeneratorScriptArgument(const QString &value = QString());

    QString value;
    unsigned flags;
};

struct CustomWizardParameters
{
public:
    enum ParseResult { ParseOk, ParseDisabled, ParseFailed };

    CustomWizardParameters();
    void clear();
    ParseResult parse(QIODevice &device, const QString &configFileFullPath,
                      Core::BaseFileWizardParameters *bp, QString *errorMessage);
    ParseResult parse(const QString &configFileFullPath,
                      Core::BaseFileWizardParameters *bp, QString *errorMessage);
    QString toString() const;

    QString directory;
    QString klass;
    QList<CustomWizardFile> files;
    QStringList filesGeneratorScript; // Complete binary, such as 'cmd /c myscript.pl'.
    QString filesGeneratorScriptWorkingDirectory;
    QList<GeneratorScriptArgument> filesGeneratorScriptArguments;

    QString fieldPageTitle;
    QList<CustomWizardField> fields;
    QList<CustomWizardValidationRule> rules;
    int firstPageId;
};

// Documentation inside.
struct CustomWizardContext {
    typedef QMap<QString, QString> FieldReplacementMap;
    typedef QSharedPointer<QTemporaryFile> TemporaryFilePtr;
    typedef QList<TemporaryFilePtr> TemporaryFilePtrList;

    void reset();

    static bool replaceFields(const FieldReplacementMap &fm, QString *s);
    static bool replaceFields(const FieldReplacementMap &fm, QString *s,
                              TemporaryFilePtrList *files);

    static QString processFile(const FieldReplacementMap &fm, QString in);

    FieldReplacementMap baseReplacements;
    FieldReplacementMap replacements;

    QString path;
    // Where files should be created, that is, 'path' for simple wizards
    // or "path + project" for project wizards.
    QString targetPath;
};

extern const char customWizardFileOpenEditorAttributeC[];
extern const char customWizardFileOpenProjectAttributeC[];

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMPROJECTWIZARDPARAMETERS_H
