TEMPLATE = lib
TARGET = VCSBase
DEFINES += VCSBASE_LIBRARY
include(../../qtcreatorplugin.pri)
include(vcsbase_dependencies.pri)
HEADERS += vcsbase_global.h \
    vcsbaseconstants.h \
    vcsconfigurationpage.h \
    vcsplugin.h \
    corelistener.h \
    vcsbaseplugin.h \
    baseannotationhighlighter.h \
    diffhighlighter.h \
    vcsbasetextdocument.h \
    vcsbaseeditor.h \
    vcsbasesubmiteditor.h \
    basevcseditorfactory.h \
    submiteditorfile.h \
    basevcssubmiteditorfactory.h \
    submitfilemodel.h \
    commonvcssettings.h \
    commonsettingspage.h \
    nicknamedialog.h \
    basecheckoutwizard.h \
    checkoutwizarddialog.h \
    checkoutprogresswizardpage.h \
    checkoutjobs.h \
    basecheckoutwizardpage.h \
    vcsbaseoutputwindow.h \
    cleandialog.h \
    vcsbaseoptionspage.h \
    vcsjobrunner.h \
    vcsbaseclient.h \
    vcsbaseclientsettings.h \
    vcsbaseeditorparameterwidget.h

SOURCES += vcsplugin.cpp \
    vcsbaseplugin.cpp \
    vcsconfigurationpage.cpp \
    corelistener.cpp \
    baseannotationhighlighter.cpp \
    diffhighlighter.cpp \
    vcsbasetextdocument.cpp \
    vcsbaseeditor.cpp \
    vcsbasesubmiteditor.cpp \
    basevcseditorfactory.cpp \
    submiteditorfile.cpp \
    basevcssubmiteditorfactory.cpp \
    submitfilemodel.cpp \
    commonvcssettings.cpp \
    commonsettingspage.cpp \
    nicknamedialog.cpp \
    basecheckoutwizard.cpp \
    checkoutwizarddialog.cpp \
    checkoutprogresswizardpage.cpp \
    checkoutjobs.cpp \
    basecheckoutwizardpage.cpp \
    vcsbaseoutputwindow.cpp \
    cleandialog.cpp \
    vcsbaseoptionspage.cpp \
    vcsjobrunner.cpp \
    vcsbaseclient.cpp \
    vcsbaseclientsettings.cpp \
    vcsbaseeditorparameterwidget.cpp

RESOURCES += vcsbase.qrc

FORMS += commonsettingspage.ui \
    nicknamedialog.ui \
    checkoutprogresswizardpage.ui \
    basecheckoutwizardpage.ui \
    cleandialog.ui \
    vcsconfigurationpage.ui \

