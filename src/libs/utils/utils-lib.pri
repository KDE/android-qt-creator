dll {
    DEFINES += QTCREATOR_UTILS_LIB
} else {
    DEFINES += QTCREATOR_UTILS_STATIC_LIB
}

INCLUDEPATH += $$PWD
QT += network


win32-msvc* {
    # disable warnings caused by botan headers
    QMAKE_CXXFLAGS += -wd4250 -wd4290
}

SOURCES += $$PWD/environment.cpp \
    $$PWD/environmentmodel.cpp \
    $$PWD/qtcprocess.cpp \
    $$PWD/reloadpromptutils.cpp \
    $$PWD/stringutils.cpp \
    $$PWD/filesearch.cpp \
    $$PWD/pathchooser.cpp \
    $$PWD/pathlisteditor.cpp \
    $$PWD/wizard.cpp \
    $$PWD/filewizardpage.cpp \
    $$PWD/filewizarddialog.cpp \
    $$PWD/filesystemwatcher.cpp \
    $$PWD/projectintropage.cpp \
    $$PWD/basevalidatinglineedit.cpp \
    $$PWD/filenamevalidatinglineedit.cpp \
    $$PWD/projectnamevalidatinglineedit.cpp \
    $$PWD/codegeneration.cpp \
    $$PWD/newclasswidget.cpp \
    $$PWD/classnamevalidatinglineedit.cpp \
    $$PWD/linecolumnlabel.cpp \
    $$PWD/fancylineedit.cpp \
    $$PWD/qtcolorbutton.cpp \
    $$PWD/savedaction.cpp \
    $$PWD/submiteditorwidget.cpp \
    $$PWD/synchronousprocess.cpp \
    $$PWD/savefile.cpp \
    $$PWD/fileutils.cpp \
    $$PWD/submitfieldwidget.cpp \
    $$PWD/consoleprocess.cpp \
    $$PWD/uncommentselection.cpp \
    $$PWD/parameteraction.cpp \
    $$PWD/treewidgetcolumnstretcher.cpp \
    $$PWD/checkablemessagebox.cpp \
    $$PWD/styledbar.cpp \
    $$PWD/stylehelper.cpp \
    $$PWD/iwelcomepage.cpp \
    $$PWD/fancymainwindow.cpp \
    $$PWD/detailsbutton.cpp \
    $$PWD/detailswidget.cpp \
    $$PWD/changeset.cpp \
    $$PWD/filterlineedit.cpp \
    $$PWD/faketooltip.cpp \
    $$PWD/htmldocextractor.cpp \
    $$PWD/navigationtreeview.cpp \
    $$PWD/crumblepath.cpp \
    $$PWD/debuggerlanguagechooser.cpp \
    $$PWD/historycompleter.cpp \
    $$PWD/buildablehelperlibrary.cpp \
    $$PWD/annotateditemdelegate.cpp \
    $$PWD/fileinprojectfinder.cpp \
    $$PWD/ipaddresslineedit.cpp \
    $$PWD/statuslabel.cpp \
    $$PWD/ssh/sshsendfacility.cpp \
    $$PWD/ssh/sshremoteprocess.cpp \
    $$PWD/ssh/sshpacketparser.cpp \
    $$PWD/ssh/sshpacket.cpp \
    $$PWD/ssh/sshoutgoingpacket.cpp \
    $$PWD/ssh/sshkeygenerator.cpp \
    $$PWD/ssh/sshkeyexchange.cpp \
    $$PWD/ssh/sshincomingpacket.cpp \
    $$PWD/ssh/sshcryptofacility.cpp \
    $$PWD/ssh/sshconnection.cpp \
    $$PWD/ssh/sshchannelmanager.cpp \
    $$PWD/ssh/sshchannel.cpp \
    $$PWD/ssh/sshcapabilities.cpp \
    $$PWD/ssh/sftppacket.cpp \
    $$PWD/ssh/sftpoutgoingpacket.cpp \
    $$PWD/ssh/sftpoperation.cpp \
    $$PWD/ssh/sftpincomingpacket.cpp \
    $$PWD/ssh/sftpdefs.cpp \
    $$PWD/ssh/sftpchannel.cpp \
    $$PWD/ssh/sshremoteprocessrunner.cpp \
    $$PWD/ssh/sshconnectionmanager.cpp \
    $$PWD/outputformatter.cpp \
    $$PWD/flowlayout.cpp

win32 {
    SOURCES += \
        $$PWD/consoleprocess_win.cpp \
        $$PWD/winutils.cpp
    HEADERS += $$PWD/winutils.h
}
else:SOURCES += $$PWD/consoleprocess_unix.cpp

unix:!macx {
    HEADERS += $$PWD/unixutils.h
    SOURCES += $$PWD/unixutils.cpp
}
HEADERS += $$PWD/environment.h \
    $$PWD/environmentmodel.h \
    $$PWD/qtcprocess.h \
    $$PWD/utils_global.h \
    $$PWD/reloadpromptutils.h \
    $$PWD/stringutils.h \
    $$PWD/filesearch.h \
    $$PWD/listutils.h \
    $$PWD/pathchooser.h \
    $$PWD/pathlisteditor.h \
    $$PWD/wizard.h \
    $$PWD/filewizardpage.h \
    $$PWD/filewizarddialog.h \
    $$PWD/filesystemwatcher.h \
    $$PWD/projectintropage.h \
    $$PWD/basevalidatinglineedit.h \
    $$PWD/filenamevalidatinglineedit.h \
    $$PWD/projectnamevalidatinglineedit.h \
    $$PWD/codegeneration.h \
    $$PWD/newclasswidget.h \
    $$PWD/classnamevalidatinglineedit.h \
    $$PWD/linecolumnlabel.h \
    $$PWD/fancylineedit.h \
    $$PWD/qtcolorbutton.h \
    $$PWD/savedaction.h \
    $$PWD/submiteditorwidget.h \
    $$PWD/consoleprocess.h \
    $$PWD/consoleprocess_p.h \
    $$PWD/synchronousprocess.h \
    $$PWD/savefile.h \
    $$PWD/fileutils.h \
    $$PWD/submitfieldwidget.h \
    $$PWD/uncommentselection.h \
    $$PWD/parameteraction.h \
    $$PWD/treewidgetcolumnstretcher.h \
    $$PWD/checkablemessagebox.h \
    $$PWD/qtcassert.h \
    $$PWD/styledbar.h \
    $$PWD/stylehelper.h \
    $$PWD/iwelcomepage.h \
    $$PWD/fancymainwindow.h \
    $$PWD/detailsbutton.h \
    $$PWD/detailswidget.h \
    $$PWD/changeset.h \
    $$PWD/filterlineedit.h \
    $$PWD/faketooltip.h \
    $$PWD/htmldocextractor.h \
    $$PWD/navigationtreeview.h \
    $$PWD/crumblepath.h \
    $$PWD/debuggerlanguagechooser.h \
    $$PWD/historycompleter.h \
    $$PWD/buildablehelperlibrary.h \
    $$PWD/annotateditemdelegate.h \
    $$PWD/fileinprojectfinder.h \
    $$PWD/ipaddresslineedit.h \
    $$PWD/ssh/sshsendfacility_p.h \
    $$PWD/ssh/sshremoteprocess.h \
    $$PWD/ssh/sshremoteprocess_p.h \
    $$PWD/ssh/sshpacketparser_p.h \
    $$PWD/ssh/sshpacket_p.h \
    $$PWD/ssh/sshoutgoingpacket_p.h \
    $$PWD/ssh/sshkeygenerator.h \
    $$PWD/ssh/sshkeyexchange_p.h \
    $$PWD/ssh/sshincomingpacket_p.h \
    $$PWD/ssh/sshexception_p.h \
    $$PWD/ssh/ssherrors.h \
    $$PWD/ssh/sshcryptofacility_p.h \
    $$PWD/ssh/sshconnection.h \
    $$PWD/ssh/sshconnection_p.h \
    $$PWD/ssh/sshchannelmanager_p.h \
    $$PWD/ssh/sshchannel_p.h \
    $$PWD/ssh/sshcapabilities_p.h \
    $$PWD/ssh/sshbotanconversions_p.h \
    $$PWD/ssh/sftppacket_p.h \
    $$PWD/ssh/sftpoutgoingpacket_p.h \
    $$PWD/ssh/sftpoperation_p.h \
    $$PWD/ssh/sftpincomingpacket_p.h \
    $$PWD/ssh/sftpdefs.h \
    $$PWD/ssh/sftpchannel.h \
    $$PWD/ssh/sftpchannel_p.h \
    $$PWD/ssh/sshremoteprocessrunner.h \
    $$PWD/ssh/sshconnectionmanager.h \
    $$PWD/ssh/sshpseudoterminal.h \
    $$PWD/statuslabel.h \
    $$PWD/outputformatter.h \
    $$PWD/outputformat.h \
    $$PWD/flowlayout.h

FORMS += $$PWD/filewizardpage.ui \
    $$PWD/projectintropage.ui \
    $$PWD/newclasswidget.ui \
    $$PWD/submiteditorwidget.ui \
    $$PWD/checkablemessagebox.ui
RESOURCES += $$PWD/utils.qrc
