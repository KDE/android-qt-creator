TEMPLATE = lib
TARGET = Designer
DEFINES += DESIGNER_LIBRARY

include(../../qtcreatorplugin.pri)
include(../../shared/designerintegrationv2/designerintegration.pri)
include(cpp/cpp.pri)
include(designer_dependencies.pri)

INCLUDEPATH += ../../tools/utils

minQtVersion(5, 0, 0) {
    CONFIG += designer
#   -- Fixme: Make tools available
    INCLUDEPATH += $$QMAKE_INCDIR_QT/../../qttools/include
} else {
    # -- figure out shared dir location
    !exists($$[QT_INSTALL_HEADERS]/QtDesigner/private/qdesigner_integration_p.h) {
        QT_SOURCE_TREE=$$fromfile($$(QTDIR)/.qmake.cache,QT_SOURCE_TREE)
        INCLUDEPATH += $$QT_SOURCE_TREE/include
    }
    INCLUDEPATH += $$QMAKE_INCDIR_QT/QtDesigner
    qtAddLibrary(QtDesigner)
}

QT += xml

qtAddLibrary(QtDesignerComponents)

HEADERS += formeditorplugin.h \
        formeditorfactory.h \
        formwindoweditor.h \
        formwindowfile.h \
        formwizard.h \
        qtcreatorintegration.h \
        designerconstants.h \
        settingspage.h \
        editorwidget.h \
        formeditorw.h \
        settingsmanager.h \
        formtemplatewizardpage.h \
        formwizarddialog.h \
        codemodelhelpers.h \
        designer_export.h \
    designerxmleditor.h \
    designercontext.h \
    formeditorstack.h \
    editordata.h \
    resourcehandler.h \
    qtdesignerformclasscodegenerator.h

SOURCES += formeditorplugin.cpp \
        formeditorfactory.cpp \
        formwindoweditor.cpp \
        formwindowfile.cpp \
        formwizard.cpp \
        qtcreatorintegration.cpp \
        settingspage.cpp \
        editorwidget.cpp \
        formeditorw.cpp \
        settingsmanager.cpp \
        formtemplatewizardpage.cpp \
        formwizarddialog.cpp \
        codemodelhelpers.cpp \
    designerxmleditor.cpp \
    designercontext.cpp \
    formeditorstack.cpp \
    resourcehandler.cpp \
    qtdesignerformclasscodegenerator.cpp

RESOURCES += designer.qrc

OTHER_FILES += Designer.mimetypes.xml README.txt
