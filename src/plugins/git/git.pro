TEMPLATE = lib
TARGET = ScmGit
include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/vcsbase/vcsbase.pri)
include(../../libs/utils/utils.pri)
HEADERS += gitplugin.h \
    gitconstants.h \
    gitclient.h \
    changeselectiondialog.h \
    commitdata.h \
    settingspage.h \
    giteditor.h \
    annotationhighlighter.h \
    gitsubmiteditorwidget.h \
    gitsubmiteditor.h \
    gitversioncontrol.h \
    gitsettings.h \
    branchdialog.h \
    branchmodel.h \
    gitcommand.h \
    clonewizard.h \
    clonewizardpage.h \
    stashdialog.h \
    gitutils.h \
    remotemodel.h \
    remotedialog.h \
    branchadddialog.h

SOURCES += gitplugin.cpp \
    gitclient.cpp \
    changeselectiondialog.cpp \
    commitdata.cpp \
    settingspage.cpp \
    giteditor.cpp \
    annotationhighlighter.cpp \
    gitsubmiteditorwidget.cpp \
    gitsubmiteditor.cpp \
    gitversioncontrol.cpp \
    gitsettings.cpp \
    branchdialog.cpp \
    branchmodel.cpp \
    gitcommand.cpp \
    clonewizard.cpp \
    clonewizardpage.cpp \
    stashdialog.cpp \
    gitutils.cpp \
    remotemodel.cpp \
    remotedialog.cpp \
    branchadddialog.cpp

FORMS += changeselectiondialog.ui \
    settingspage.ui \
    gitsubmitpanel.ui \
    branchdialog.ui \
    stashdialog.ui \
    remotedialog.ui \
    remoteadditiondialog.ui \
    branchadddialog.ui

include(gitorious/gitorious.pri)

RESOURCES += \
    git.qrc



