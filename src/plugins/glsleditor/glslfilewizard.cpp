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

#include "glsleditorconstants.h"
#include "glslfilewizard.h"

#include <utils/filewizarddialog.h>
#include <utils/qtcassert.h>
#include <utils/filewizarddialog.h>

#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtGui/QWizard>
#include <QtGui/QPushButton>

namespace {
class GLSLFileWizardDialog : public Utils::FileWizardDialog
{
    Q_OBJECT
public:
    GLSLFileWizardDialog(QWidget *parent = 0)
        : Utils::FileWizardDialog(parent)
    {
    }
};
} // anonymous namespace

using namespace GLSLEditor;

GLSLFileWizard::GLSLFileWizard(const BaseFileWizardParameters &parameters,
                               ShaderType shaderType, QObject *parent):
    Core::BaseFileWizard(parameters, parent),
    m_shaderType(shaderType)
{
}

Core::GeneratedFiles GLSLFileWizard::generateFiles(const QWizard *w,
                                                 QString * /*errorMessage*/) const
{
    const GLSLFileWizardDialog *wizardDialog = qobject_cast<const GLSLFileWizardDialog *>(w);
    const QString path = wizardDialog->path();
    const QString name = wizardDialog->fileName();

    const QString fileName = Core::BaseFileWizard::buildFileName(path, name, preferredSuffix(m_shaderType));

    Core::GeneratedFile file(fileName);
    file.setContents(fileContents(fileName, m_shaderType));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << file;
}

QString GLSLFileWizard::fileContents(const QString &, ShaderType shaderType) const
{
    QString contents;
    QTextStream str(&contents);

    switch (shaderType) {
    case GLSLFileWizard::VertexShaderES:
        str << QLatin1String("attribute highp vec4 qt_Vertex;\n")
            << QLatin1String("attribute highp vec4 qt_MultiTexCoord0;\n")
            << QLatin1String("uniform highp mat4 qt_ModelViewProjectionMatrix;\n")
            << QLatin1String("varying highp vec4 qt_TexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;\n")
            << QLatin1String("    qt_TexCoord0 = qt_MultiTexCoord0;\n")
            << QLatin1String("}\n");
        break;
    case GLSLFileWizard::FragmentShaderES:
        str << QLatin1String("uniform sampler2D qt_Texture0;\n")
            << QLatin1String("varying highp vec4 qt_TexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_FragColor = texture2D(qt_Texture0, qt_TexCoord0.st);\n")
            << QLatin1String("}\n");
        break;
    case GLSLFileWizard::VertexShaderDesktop:
        str << QLatin1String("attribute vec4 qt_Vertex;\n")
            << QLatin1String("attribute vec4 qt_MultiTexCoord0;\n")
            << QLatin1String("uniform mat4 qt_ModelViewProjectionMatrix;\n")
            << QLatin1String("varying vec4 qt_TexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;\n")
            << QLatin1String("    qt_TexCoord0 = qt_MultiTexCoord0;\n")
            << QLatin1String("}\n");
        break;
    case GLSLFileWizard::FragmentShaderDesktop:
        str << QLatin1String("uniform sampler2D qt_Texture0;\n")
            << QLatin1String("varying vec4 qt_TexCoord0;\n")
            << QLatin1String("\n")
            << QLatin1String("void main(void)\n")
            << QLatin1String("{\n")
            << QLatin1String("    gl_FragColor = texture2D(qt_Texture0, qt_TexCoord0.st);\n")
            << QLatin1String("}\n");
        break;
    default: break;
    }

    return contents;
}

QWizard *GLSLFileWizard::createWizardDialog(QWidget *parent, const QString &defaultPath,
                                          const WizardPageList &extensionPages) const
{
    GLSLFileWizardDialog *wizardDialog = new GLSLFileWizardDialog(parent);
    wizardDialog->setWindowTitle(tr("New %1").arg(displayName()));
    setupWizard(wizardDialog);
    wizardDialog->setPath(defaultPath);
    foreach (QWizardPage *p, extensionPages)
        BaseFileWizard::applyExtensionPageShortTitle(wizardDialog, wizardDialog->addPage(p));
    return wizardDialog;
}

QString GLSLFileWizard::preferredSuffix(ShaderType shaderType) const
{
    switch (shaderType) {
    case GLSLFileWizard::VertexShaderES:
        return QLatin1String("vsh");
    case GLSLFileWizard::FragmentShaderES:
        return QLatin1String("fsh");
    case GLSLFileWizard::VertexShaderDesktop:
        return QLatin1String("vert");
    case GLSLFileWizard::FragmentShaderDesktop:
        return QLatin1String("frag");
    default:
        return QLatin1String("glsl");
    }
}

#include "glslfilewizard.moc"
