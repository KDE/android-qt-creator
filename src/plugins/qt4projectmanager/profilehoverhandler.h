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

#ifndef PROFILEHOVERHANDLER_H
#define PROFILEHOVERHANDLER_H

#include <texteditor/basehoverhandler.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace TextEditor {
class ITextEditor;
}

namespace Qt4ProjectManager {
namespace Internal {

class ProFileHoverHandler : public TextEditor::BaseHoverHandler
{
    Q_OBJECT
public:
    ProFileHoverHandler(QObject *parent = 0);
    virtual ~ProFileHoverHandler();

signals:
    void creatorHelpRequested(const QUrl &url);

private:
    virtual bool acceptEditor(Core::IEditor *editor);
    virtual void identifyMatch(TextEditor::ITextEditor *editor, int pos);
    void identifyQMakeKeyword(const QString &text, int pos);

    enum ManualKind {
        VariableManual,
        FunctionManual,
        UnknownManual
    };

    QString manualName() const;
    void identifyDocFragment(ManualKind manualKind,
                       const QString &keyword);

    QString m_docFragment;
    ManualKind m_manualKind;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEHOVERHANDLER_H
