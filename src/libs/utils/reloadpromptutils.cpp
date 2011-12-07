/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "reloadpromptutils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QAbstractButton>

using namespace Utils;

QTCREATOR_UTILS_EXPORT Utils::ReloadPromptAnswer
    Utils::reloadPrompt(const QString &fileName, bool modified, QWidget *parent)
{

    const QString title = QCoreApplication::translate("Utils::reloadPrompt", "File Changed");
    QString msg;

    if (modified)
        msg = QCoreApplication::translate("Utils::reloadPrompt",
                                          "The unsaved file <i>%1</i> has been changed outside Qt Creator. Do you want to reload it and discard your changes?");
    else
        msg = QCoreApplication::translate("Utils::reloadPrompt",
                                          "The file <i>%1</i> has changed outside Qt Creator. Do you want to reload it?");
    msg = msg.arg(QFileInfo(fileName).fileName());
    return reloadPrompt(title, msg, QDir::toNativeSeparators(fileName), parent);
}

QTCREATOR_UTILS_EXPORT Utils::ReloadPromptAnswer
    Utils::reloadPrompt(const QString &title, const QString &prompt, const QString &details, QWidget *parent)
{
    QMessageBox msg(parent);
    msg.setStandardButtons(QMessageBox::Yes|QMessageBox::YesToAll|QMessageBox::No|QMessageBox::NoToAll);
    msg.setDefaultButton(QMessageBox::YesToAll);
    msg.setWindowTitle(title);
    msg.setText(prompt);
    msg.setDetailedText(details);

    switch (msg.exec()) {
    case QMessageBox::Yes:
        return  ReloadCurrent;
    case QMessageBox::YesToAll:
        return ReloadAll;
    case QMessageBox::No:
        return ReloadSkipCurrent;
    default:
        break;
    }
    return ReloadNone;
}

QTCREATOR_UTILS_EXPORT Utils::FileDeletedPromptAnswer
        Utils::fileDeletedPrompt(const QString &fileName, bool triggerExternally, QWidget *parent)
{
    const QString title = QCoreApplication::translate("Utils::fileDeletedPrompt", "File has been removed");
    QString msg;
    if (triggerExternally)
        msg = QCoreApplication::translate("Utils::fileDeletedPrompt",
                                          "The file %1 has been removed outside Qt Creator. Do you want to save it under a different name, or close the editor?").arg(QDir::toNativeSeparators(fileName));
    else
        msg = QCoreApplication::translate("Utils::fileDeletedPrompt",
                                          "The file %1 was removed. Do you want to save it under a different name, or close the editor?").arg(QDir::toNativeSeparators(fileName));
    QMessageBox box(QMessageBox::Question, title, msg, QMessageBox::NoButton, parent);
    QPushButton *close = box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "&Close"), QMessageBox::RejectRole);
    QPushButton *saveas = box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "Save &as..."), QMessageBox::ActionRole);
    QPushButton *save = box.addButton(QCoreApplication::translate("Utils::fileDeletedPrompt", "&Save"), QMessageBox::AcceptRole);
    box.setDefaultButton(saveas);
    box.exec();
    QAbstractButton *clickedbutton = box.clickedButton();
    if (clickedbutton == close) {
        return FileDeletedClose;
    } else if (clickedbutton == saveas) {
        return FileDeletedSaveAs;
    } else if (clickedbutton == save) {
        return FileDeletedSave;
    }
    return FileDeletedClose;
}
