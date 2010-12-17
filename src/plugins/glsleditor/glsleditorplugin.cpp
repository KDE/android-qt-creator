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

#include "glsleditorplugin.h"
#include "glsleditor.h"
#include "glsleditorconstants.h"
#include "glsleditorfactory.h"
#include "glslcodecompletion.h"
#include "glslfilewizard.h"
#include "glslhoverhandler.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/taskhub.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textfilewizard.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/completionsupport.h>
#include <utils/qtcassert.h>

#include <glsl/glslengine.h>
#include <glsl/glslparser.h>
#include <glsl/glsllexer.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QAction>

using namespace GLSLEditor;
using namespace GLSLEditor::Internal;
using namespace GLSLEditor::Constants;

GLSLEditorPlugin *GLSLEditorPlugin::m_instance = 0;

GLSLEditorPlugin::InitFile::~InitFile()
{
    delete engine;
}

GLSLEditorPlugin::GLSLEditorPlugin() :
    m_editor(0),
    m_actionHandler(0)
{
    m_instance = this;
}

GLSLEditorPlugin::~GLSLEditorPlugin()
{
    removeObject(m_editor);
    delete m_actionHandler;
    m_instance = 0;
}

/*! Copied from cppplugin.cpp */
static inline
Core::Command *createSeparator(Core::ActionManager *am,
                               QObject *parent,
                               Core::Context &context,
                               const char *id)
{
    QAction *separator = new QAction(parent);
    separator->setSeparator(true);
    return am->registerAction(separator, Core::Id(id), context);
}

bool GLSLEditorPlugin::initialize(const QStringList & /*arguments*/, QString *error_message)
{
    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/glsleditor/GLSLEditor.mimetypes.xml"), error_message))
        return false;

    parseGlslFile(QLatin1String("glsl_120.frag"), &m_glsl_120_frag);
    parseGlslFile(QLatin1String("glsl_120.vert"), &m_glsl_120_vert);
    parseGlslFile(QLatin1String("glsl_120_common.glsl"), &m_glsl_120_common);
    parseGlslFile(QLatin1String("glsl_es_100.frag"), &m_glsl_es_100_frag);
    parseGlslFile(QLatin1String("glsl_es_100.vert"), &m_glsl_es_100_vert);
    parseGlslFile(QLatin1String("glsl_es_100_common.glsl"), &m_glsl_es_100_common);


//    m_modelManager = new ModelManager(this);
//    addAutoReleasedObject(m_modelManager);

    addAutoReleasedObject(new GLSLHoverHandler(this));

    Core::Context context(GLSLEditor::Constants::C_GLSLEDITOR_ID);

    m_editor = new GLSLEditorFactory(this);
    addObject(m_editor);

    CodeCompletion *completion = new CodeCompletion(this);
    addAutoReleasedObject(completion);

    m_actionHandler = new TextEditor::TextEditorActionHandler(GLSLEditor::Constants::C_GLSLEDITOR_ID,
                                                              TextEditor::TextEditorActionHandler::Format
                                                              | TextEditor::TextEditorActionHandler::UnCommentSelection
                                                              | TextEditor::TextEditorActionHandler::UnCollapseAll);
    m_actionHandler->initializeActions();

    Core::ActionManager *am =  core->actionManager();
    Core::ActionContainer *contextMenu = am->createMenu(GLSLEditor::Constants::M_CONTEXT);
    Core::ActionContainer *glslToolsMenu = am->createMenu(Core::Id(Constants::M_TOOLS_GLSL));
    glslToolsMenu->setOnAllDisabledBehavior(Core::ActionContainer::Hide);
    QMenu *menu = glslToolsMenu->menu();
    //: GLSL sub-menu in the Tools menu
    menu->setTitle(tr("GLSL"));
    am->actionContainer(Core::Constants::M_TOOLS)->addMenu(glslToolsMenu);

    Core::Command *cmd = 0;

    // Insert marker for "Refactoring" menu:
    Core::Context globalContext(Core::Constants::C_GLOBAL);
    Core::Command *sep = createSeparator(am, this, globalContext,
                                         Constants::SEPARATOR1);
    sep->action()->setObjectName(Constants::M_REFACTORING_MENU_INSERTION_POINT);
    contextMenu->addAction(sep);
    contextMenu->addAction(createSeparator(am, this, globalContext,
                                           Constants::SEPARATOR2));

    cmd = am->command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    // Set completion settings and keep them up to date
    TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
    completion->setCompletionSettings(textEditorSettings->completionSettings());
    connect(textEditorSettings, SIGNAL(completionSettingsChanged(TextEditor::CompletionSettings)),
            completion, SLOT(setCompletionSettings(TextEditor::CompletionSettings)));

    error_message->clear();

    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    Core::MimeDatabase *mimeDatabase = Core::ICore::instance()->mimeDatabase();
    iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/glsleditor/images/glslfile.png")),
                                                 mimeDatabase->findByType(QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE)));
    iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/glsleditor/images/glslfile.png")),
                                                 mimeDatabase->findByType(QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_VERT)));
    iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/glsleditor/images/glslfile.png")),
                                                 mimeDatabase->findByType(QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG)));
    iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/glsleditor/images/glslfile.png")),
                                                 mimeDatabase->findByType(QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_VERT_ES)));
    iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/glsleditor/images/glslfile.png")),
                                                 mimeDatabase->findByType(QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG_ES)));

    Core::BaseFileWizardParameters fragWizardParameters(Core::IWizard::FileWizard);
    fragWizardParameters.setCategory(QLatin1String(Constants::WIZARD_CATEGORY_GLSL));
    fragWizardParameters.setDisplayCategory(QCoreApplication::translate("GLSLEditor", Constants::WIZARD_TR_CATEGORY_GLSL));
    fragWizardParameters.setDescription
        (tr("Creates a fragment shader in the OpenGL/ES 2.0 Shading "
            "Language (GLSL/ES).  Fragment shaders generate the final "
            "pixel colors for triangles, points, and lines rendered "
            "with OpenGL."));
    fragWizardParameters.setDisplayName(tr("Fragment shader (OpenGL/ES 2.0)"));
    fragWizardParameters.setId(QLatin1String("F.GLSL"));
    addAutoReleasedObject(new GLSLFileWizard(fragWizardParameters, GLSLFileWizard::FragmentShaderES, core));

    Core::BaseFileWizardParameters vertWizardParameters(Core::IWizard::FileWizard);
    vertWizardParameters.setCategory(QLatin1String(Constants::WIZARD_CATEGORY_GLSL));
    vertWizardParameters.setDisplayCategory(QCoreApplication::translate("GLSLEditor", Constants::WIZARD_TR_CATEGORY_GLSL));
    vertWizardParameters.setDescription
        (tr("Creates a vertex shader in the OpenGL/ES 2.0 Shading "
            "Language (GLSL/ES).  Vertex shaders transform the "
            "positions, normals, and texture co-ordinates of "
            "triangles, points, and lines rendered with OpenGL."));
    vertWizardParameters.setDisplayName(tr("Vertex shader (OpenGL/ES 2.0)"));
    vertWizardParameters.setId(QLatin1String("G.GLSL"));
    addAutoReleasedObject(new GLSLFileWizard(vertWizardParameters, GLSLFileWizard::VertexShaderES, core));

    fragWizardParameters.setDescription
        (tr("Creates a fragment shader in the Desktop OpenGL Shading "
            "Language (GLSL).  Fragment shaders generate the final "
            "pixel colors for triangles, points, and lines rendered "
            "with OpenGL."));
    fragWizardParameters.setDisplayName(tr("Fragment shader (Desktop OpenGL)"));
    fragWizardParameters.setId(QLatin1String("J.GLSL"));
    addAutoReleasedObject(new GLSLFileWizard(fragWizardParameters, GLSLFileWizard::FragmentShaderDesktop, core));

    vertWizardParameters.setDescription
        (tr("Creates a vertex shader in the Desktop OpenGL Shading "
            "Language (GLSL).  Vertex shaders transform the "
            "positions, normals, and texture co-ordinates of "
            "triangles, points, and lines rendered with OpenGL."));
    vertWizardParameters.setDisplayName(tr("Vertex shader (Desktop OpenGL)"));
    vertWizardParameters.setId(QLatin1String("K.GLSL"));
    addAutoReleasedObject(new GLSLFileWizard(vertWizardParameters, GLSLFileWizard::VertexShaderDesktop, core));

    return true;
}

void GLSLEditorPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag GLSLEditorPlugin::aboutToShutdown()
{
    // delete GLSL::Icons::instance(); // delete object held by singleton

    return IPlugin::aboutToShutdown();
}

void GLSLEditorPlugin::initializeEditor(GLSLEditor::GLSLTextEditor *editor)
{
    QTC_ASSERT(m_instance, /**/);

    m_actionHandler->setupActions(editor);

    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);

//    // auto completion
    connect(editor, SIGNAL(requestAutoCompletion(TextEditor::ITextEditable*, bool)),
            TextEditor::CompletionSupport::instance(), SLOT(autoComplete(TextEditor::ITextEditable*, bool)));

//    // quick fix
//    connect(editor, SIGNAL(requestQuickFix(TextEditor::ITextEditable*)),
//            this, SLOT(quickFix(TextEditor::ITextEditable*)));
}


Core::Command *GLSLEditorPlugin::addToolAction(QAction *a, Core::ActionManager *am,
                                               Core::Context &context, const QString &name,
                                               Core::ActionContainer *c1, const QString &keySequence)
{
    Core::Command *command = am->registerAction(a, name, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    c1->addAction(command);
    return command;
}

QByteArray GLSLEditorPlugin::glslFile(const QString &fileName)
{
    QString path = Core::ICore::instance()->resourcePath();
    path += QLatin1String("/glsl/");
    path += fileName;
    QFile file(path);
    if (file.open(QFile::ReadOnly))
        return file.readAll();
    return QByteArray();
}

void GLSLEditorPlugin::parseGlslFile(const QString &fileName, InitFile *initFile)
{
    // Parse the builtins for any langugage variant so we can use all keywords.
    const unsigned variant = GLSL::Lexer::Variant_All;

    const QByteArray code = glslFile(fileName);
    initFile->engine = new GLSL::Engine();
    GLSL::Parser parser(initFile->engine, code.constData(), code.size(), variant);
    initFile->ast = parser.parse();
}

const GLSLEditorPlugin::InitFile *GLSLEditorPlugin::fragmentShaderInit(int variant) const
{
    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return &m_glsl_120_frag;
    else
        return &m_glsl_es_100_frag;
}

const GLSLEditorPlugin::InitFile *GLSLEditorPlugin::vertexShaderInit(int variant) const
{
    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return &m_glsl_120_vert;
    else
        return &m_glsl_es_100_vert;
}

const GLSLEditorPlugin::InitFile *GLSLEditorPlugin::shaderInit(int variant) const
{
    if (variant & GLSL::Lexer::Variant_GLSL_120)
        return &m_glsl_120_common;
    else
        return &m_glsl_es_100_common;
}

Q_EXPORT_PLUGIN(GLSLEditorPlugin)
