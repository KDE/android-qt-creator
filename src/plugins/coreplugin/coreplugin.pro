TEMPLATE = lib
TARGET = Core
DEFINES += CORE_LIBRARY
QT += xml \
    network \
    script \
    sql
CONFIG += help
include(../../qtcreatorplugin.pri)
include(../../libs/utils/utils.pri)
include(../../shared/scriptwrapper/scriptwrapper.pri)
include(coreplugin_dependencies.pri)
win32-msvc*:QMAKE_CXXFLAGS += -wd4251 -wd4290 -wd4250
INCLUDEPATH += dialogs \
    actionmanager \
    editormanager \
    progressmanager \
    scriptmanager
DEPENDPATH += dialogs \
    actionmanager \
    editormanager \
    scriptmanager
SOURCES += mainwindow.cpp \
    editmode.cpp \
    tabpositionindicator.cpp \
    fancyactionbar.cpp \
    fancytabwidget.cpp \
    flowlayout.cpp \
    generalsettings.cpp \
    filemanager.cpp \
    uniqueidmanager.cpp \
    messagemanager.cpp \
    messageoutputwindow.cpp \
    outputpane.cpp \
    vcsmanager.cpp \
    statusbarmanager.cpp \
    versiondialog.cpp \
    editormanager/editormanager.cpp \
    editormanager/editorview.cpp \
    editormanager/openeditorsmodel.cpp \
    editormanager/openeditorsview.cpp \
    editormanager/openeditorswindow.cpp \
    editormanager/iexternaleditor.cpp \
    actionmanager/actionmanager.cpp \
    actionmanager/command.cpp \
    actionmanager/actioncontainer.cpp \
    actionmanager/commandsfile.cpp \
    dialogs/saveitemsdialog.cpp \
    dialogs/newdialog.cpp \
    dialogs/settingsdialog.cpp \
    actionmanager/commandmappings.cpp \
    dialogs/shortcutsettings.cpp \
    dialogs/openwithdialog.cpp \
    progressmanager/progressmanager.cpp \
    progressmanager/progressview.cpp \
    progressmanager/progressbar.cpp \
    progressmanager/futureprogress.cpp \
    scriptmanager/scriptmanager.cpp \
    statusbarwidget.cpp \
    coreplugin.cpp \
    variablemanager.cpp \
    modemanager.cpp \
    coreimpl.cpp \
    basefilewizard.cpp \
    plugindialog.cpp \
    inavigationwidgetfactory.cpp \
    navigationwidget.cpp \
    manhattanstyle.cpp \
    minisplitter.cpp \
    styleanimator.cpp \
    findplaceholder.cpp \
    rightpane.cpp \
    sidebar.cpp \
    fileiconprovider.cpp \
    mimedatabase.cpp \
    icore.cpp \
    editormanager/ieditor.cpp \
    dialogs/ioptionspage.cpp \
    dialogs/iwizard.cpp \
    settingsdatabase.cpp \
    eventfilteringmainwindow.cpp \
    imode.cpp \
    editormanager/systemeditor.cpp \
    designmode.cpp \
    editortoolbar.cpp \
    helpmanager.cpp \
    ssh/sshsendfacility.cpp \
    ssh/sshremoteprocess.cpp \
    ssh/sshpacketparser.cpp \
    ssh/sshpacket.cpp \
    ssh/sshoutgoingpacket.cpp \
    ssh/sshkeygenerator.cpp \
    ssh/sshkeyexchange.cpp \
    ssh/sshincomingpacket.cpp \
    ssh/sshcryptofacility.cpp \
    ssh/sshconnection.cpp \
    ssh/sshchannelmanager.cpp \
    ssh/sshchannel.cpp \
    ssh/sshcapabilities.cpp \
    ssh/sftppacket.cpp \
    ssh/sftpoutgoingpacket.cpp \
    ssh/sftpoperation.cpp \
    ssh/sftpincomingpacket.cpp \
    ssh/sftpdefs.cpp \
    ssh/sftpchannel.cpp \
    ssh/sshremoteprocessrunner.cpp \
    outputpanemanager.cpp \
    navigationsubwidget.cpp \
    sidebarwidget.cpp \
    rssfetcher.cpp

HEADERS += mainwindow.h \
    editmode.h \
    tabpositionindicator.h \
    fancyactionbar.h \
    fancytabwidget.h \
    flowlayout.h \
    generalsettings.h \
    filemanager.h \
    uniqueidmanager.h \
    messagemanager.h \
    messageoutputwindow.h \
    outputpane.h \
    vcsmanager.h \
    statusbarmanager.h \
    editormanager/editormanager.h \
    editormanager/editorview.h \
    editormanager/openeditorsmodel.h \
    editormanager/openeditorsview.h \
    editormanager/openeditorswindow.h \
    editormanager/ieditor.h \
    editormanager/iexternaleditor.h \
    editormanager/ieditorfactory.h \
    actionmanager/actioncontainer.h \
    actionmanager/actionmanager.h \
    actionmanager/command.h \
    actionmanager/actionmanager_p.h \
    actionmanager/command_p.h \
    actionmanager/actioncontainer_p.h \
    actionmanager/commandsfile.h \
    dialogs/saveitemsdialog.h \
    dialogs/newdialog.h \
    dialogs/settingsdialog.h \
    actionmanager/commandmappings.h \
    dialogs/shortcutsettings.h \
    dialogs/openwithdialog.h \
    dialogs/iwizard.h \
    dialogs/ioptionspage.h \
    progressmanager/progressmanager_p.h \
    progressmanager/progressview.h \
    progressmanager/progressbar.h \
    progressmanager/futureprogress.h \
    progressmanager/progressmanager.h \
    icontext.h \
    icore.h \
    ifile.h \
    ifilefactory.h \
    imode.h \
    ioutputpane.h \
    coreconstants.h \
    iversioncontrol.h \
    ifilewizardextension.h \
    icorelistener.h \
    versiondialog.h \
    scriptmanager/metatypedeclarations.h \
    scriptmanager/scriptmanager.h \
    scriptmanager/scriptmanager_p.h \
    core_global.h \
    statusbarwidget.h \
    coreplugin.h \
    variablemanager.h \
    modemanager.h \
    coreimpl.h \
    basefilewizard.h \
    plugindialog.h \
    inavigationwidgetfactory.h \
    navigationwidget.h \
    manhattanstyle.h \
    minisplitter.h \
    styleanimator.h \
    findplaceholder.h \
    rightpane.h \
    sidebar.h \
    fileiconprovider.h \
    mimedatabase.h \
    settingsdatabase.h \
    eventfilteringmainwindow.h \
    editormanager/systemeditor.h \
    designmode.h \
    editortoolbar.h \
    helpmanager.h \
    ssh/sshsendfacility_p.h \
    ssh/sshremoteprocess.h \
    ssh/sshremoteprocess_p.h \
    ssh/sshpacketparser_p.h \
    ssh/sshpacket_p.h \
    ssh/sshoutgoingpacket_p.h \
    ssh/sshkeygenerator.h \
    ssh/sshkeyexchange_p.h \
    ssh/sshincomingpacket_p.h \
    ssh/sshexception_p.h \
    ssh/ssherrors.h \
    ssh/sshcryptofacility_p.h \
    ssh/sshconnection.h \
    ssh/sshconnection_p.h \
    ssh/sshchannelmanager_p.h \
    ssh/sshchannel_p.h \
    ssh/sshcapabilities_p.h \
    ssh/sshbotanconversions_p.h \
    ssh/sftppacket_p.h \
    ssh/sftpoutgoingpacket_p.h \
    ssh/sftpoperation_p.h \
    ssh/sftpincomingpacket_p.h \
    ssh/sftpdefs.h \
    ssh/sftpchannel.h \
    ssh/sftpchannel_p.h \
    ssh/sshremoteprocessrunner.h \
    outputpanemanager.h \
    navigationsubwidget.h \
    sidebarwidget.h \
    rssfetcher.h

FORMS += dialogs/newdialog.ui \
    actionmanager/commandmappings.ui \
    dialogs/saveitemsdialog.ui \
    dialogs/openwithdialog.ui \
    editormanager/openeditorsview.ui \
    generalsettings.ui
RESOURCES += core.qrc \
    fancyactionbar.qrc

win32 {
    SOURCES += progressmanager/progressmanager_win.cpp
    LIBS += -lole32
}
else:macx {
    OBJECTIVE_SOURCES += progressmanager/progressmanager_mac.mm
    LIBS += -framework AppKit
}
else:unix {
    SOURCES += progressmanager/progressmanager_x11.cpp

    images.files = images/qtcreator_logo_*.png
    images.path = /share/pixmaps
    INSTALLS += images
}
OTHER_FILES += editormanager/BinFiles.mimetypes.xml ide_version.h.in

QMAKE_SUBSTITUTES += ide_version.h.in
