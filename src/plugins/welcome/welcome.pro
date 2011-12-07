TEMPLATE = lib
TARGET = Welcome
QT += network declarative

include(../../qtcreatorplugin.pri)
include(welcome_dependencies.pri)

HEADERS += welcomeplugin.h \
    welcome_global.h \
    multifeedrssmodel.h

SOURCES += welcomeplugin.cpp \
    multifeedrssmodel.cpp

DEFINES += WELCOME_LIBRARY

QML_IMPORT_PATH = $$IDE_SOURCE_TREE/lib/qtcreator/
