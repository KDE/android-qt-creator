TARGET = qml2puppet

TEMPLATE = app

QT += core gui declarative network

contains (QT_CONFIG, webkit) {
    QT += webkit
}

QT += core-private declarative-private gui-private script-private v8-private

DEFINES += QWEAKPOINTER_ENABLE_ARROW

include(../../../../qtcreator.pri)
DESTDIR = $$[QT_INSTALL_BINS]
include(../../../rpath.pri)

include (instances/instances.pri)
include (../instances/instances.pri)
include (../commands/commands.pri)
include (../container/container.pri)
include (../interfaces/interfaces.pri)

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    include($$QT_BREAKPAD_ROOT_PATH/qtbreakpad.pri)
}

SOURCES += main.cpp
RESOURCES += ../qmlpuppet.qrc

OTHER_FILES += Info.plist.in
macx {
    info.input = Info.plist.in
    info.output = $$DESTDIR/$${TARGET}.app/Contents/Info.plist
    QMAKE_SUBSTITUTES += info
} else {
    target.path  = /bin
    INSTALLS    += target
}
