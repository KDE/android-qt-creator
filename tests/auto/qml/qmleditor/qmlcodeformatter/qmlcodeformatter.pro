include(../../../qttest.pri)

SRCDIR = ../../../../../src

include($$SRCDIR/libs/qmljs/qmljs.pri)
include($$SRCDIR/libs/utils/utils.pri)
include($$SRCDIR/libs/languageutils/languageutils.pri)

SOURCES += \
    tst_qmlcodeformatter.cpp \
    $$SRCDIR/plugins/qmljstools/qmljsqtstylecodeformatter.cpp \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.cpp

HEADERS += \
    $$SRCDIR/plugins/qmljstools/qmljsqtstylecodeformatter.h \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.h

INCLUDEPATH += $$SRCDIR/plugins $$SRCDIR/libs
