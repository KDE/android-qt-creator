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

#ifndef FORMWINDOWFILE_H
#define FORMWINDOWFILE_H

#include <coreplugin/textfile.h>

#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
class QFile;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

class FormWindowFile : public Core::TextFile
{
    Q_OBJECT

public:
    explicit FormWindowFile(QDesignerFormWindowInterface *form, QObject *parent = 0);

    // IFile
    virtual bool save(QString *errorString, const QString &fileName, bool autoSave);
    virtual QString fileName() const;
    virtual bool shouldAutoSave() const;
    virtual bool isModified() const;
    virtual bool isReadOnly() const;
    virtual bool isSaveAsAllowed() const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;
    virtual void rename(const QString &newName);

    // Internal
    void setSuggestedFileName(const QString &fileName);

    bool writeFile(const QString &fileName, QString *errorString) const;

    QDesignerFormWindowInterface *formWindow() const;

signals:
    // Internal
    void saved();
    void reload(QString *errorString, const QString &);
    void setDisplayName(const QString &);

public slots:
    void setFileName(const QString &);
    void setShouldAutoSave(bool sad = true) { m_shouldAutoSave = sad; }

private slots:
    void slotFormWindowRemoved(QDesignerFormWindowInterface *w);

private:
    const QString m_mimeType;

    QString m_fileName;
    QString m_suggestedName;
    bool m_shouldAutoSave;
    // Might actually go out of scope before the IEditor due
    // to deleting the WidgetHost which owns it.
    QPointer<QDesignerFormWindowInterface> m_formWindow;
};

} // namespace Internal
} // namespace Designer

#endif // FORMWINDOWFILE_H
