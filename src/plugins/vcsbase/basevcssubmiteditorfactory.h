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

#ifndef VCSBASE_BASEEDITORFACTORY_H
#define VCSBASE_BASEEDITORFACTORY_H

#include "vcsbase_global.h"

#include <coreplugin/editormanager/ieditorfactory.h>

namespace VcsBase {

class VcsBaseSubmitEditor;
class VcsBaseSubmitEditorParameters;

namespace Internal {
class BaseVcsSubmitEditorFactoryPrivate;
} // namespace Internal

// Parametrizable base class for editor factories creating instances of
// VcsBaseSubmitEditor subclasses.
class VCSBASE_EXPORT BaseVcsSubmitEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

protected:
    explicit BaseVcsSubmitEditorFactory(const VcsBaseSubmitEditorParameters *parameters);

public:
    ~BaseVcsSubmitEditorFactory();

    Core::IEditor *createEditor(QWidget *parent);
    Core::Id id() const;
    QString displayName() const;
    QStringList mimeTypes() const;
    Core::IFile *open(const QString &fileName);

private:
    virtual VcsBaseSubmitEditor
        *createBaseSubmitEditor(const VcsBaseSubmitEditorParameters *parameters,
                                QWidget *parent) = 0;

    Internal::BaseVcsSubmitEditorFactoryPrivate *const d;
};

// Utility template to create an editor that has a constructor taking the
// parameter struct and a parent widget.

template <class Editor>
class VcsSubmitEditorFactory : public BaseVcsSubmitEditorFactory
{
public:
    explicit VcsSubmitEditorFactory(const VcsBaseSubmitEditorParameters *parameters)
        : BaseVcsSubmitEditorFactory(parameters)
    {
    }

private:
    VcsBaseSubmitEditor *createBaseSubmitEditor
        (const VcsBaseSubmitEditorParameters *parameters, QWidget *parent)
    {
        return new Editor(parameters, parent);
    }
};

} // namespace VcsBase

#endif // VCSBASE_BASEEDITOR_H
