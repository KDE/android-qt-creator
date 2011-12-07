include(../../qtcreator.pri)
include(../shared/qtsingleapplication/qtsingleapplication.pri)

TEMPLATE = app
TARGET = $$IDE_APP_TARGET
DESTDIR = $$IDE_APP_PATH

SOURCES += main.cpp

include(../rpath.pri)

LIBS *= -l$$qtLibraryName(ExtensionSystem) -l$$qtLibraryName(Aggregation)

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    include($$QT_BREAKPAD_ROOT_PATH/qtbreakpad.pri)
}
win32 {
    RC_FILE = qtcreator.rc
    target.path = /bin
    INSTALLS += target
} else:macx {
    LIBS += -framework CoreFoundation
    ICON = qtcreator.icns
    FILETYPES.files = profile.icns prifile.icns
    FILETYPES.path = Contents/Resources
    QMAKE_BUNDLE_DATA += FILETYPES
    info.input = Info.plist.in
    info.output = $$IDE_BIN_PATH/../Info.plist
    QMAKE_SUBSTITUTES = info
} else {
    target.path  = /bin
    INSTALLS    += target
}

OTHER_FILES += qtcreator.rc \
    Info.plist.in \
    $$PWD/app_version.h.in

QMAKE_SUBSTITUTES += $$PWD/app_version.h.in
