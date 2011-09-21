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

#ifndef BEHAVIORSETTINGSPAGE_H
#define BEHAVIORSETTINGSPAGE_H

#include "texteditor_global.h"

#include "texteditoroptionspage.h"

namespace TextEditor {

class TabSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;

class TabPreferences;

class BehaviorSettingsPageParameters
{
public:
    QString id;
    QString displayName;
    QString settingsPrefix;
};

class BehaviorSettingsPage : public TextEditorOptionsPage
{
    Q_OBJECT

public:
    BehaviorSettingsPage(const BehaviorSettingsPageParameters &p, QObject *parent);
    virtual ~BehaviorSettingsPage();

    // IOptionsPage
    QString id() const;
    QString displayName() const;

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish();
    bool matches(const QString &s) const;

    const StorageSettings &storageSettings() const;
    const BehaviorSettings &behaviorSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;

    TabPreferences *tabPreferences() const;

signals:
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);

private:
    void settingsFromUI(StorageSettings *storageSettings,
                        BehaviorSettings *behaviorSettings,
                        ExtraEncodingSettings *extraEncodingSettings) const;
    void settingsToUI();

    QList<QTextCodec *> m_codecs;
    struct BehaviorSettingsPagePrivate;
    BehaviorSettingsPagePrivate *d;
};

} // namespace TextEditor

#endif // BEHAVIORSETTINGSPAGE_H
