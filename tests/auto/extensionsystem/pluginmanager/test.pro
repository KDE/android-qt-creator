TARGET = pluginmanager

# Input

include(../../qttest.pri)
include(../../../../src/libs/extensionsystem/extensionsystem.pri)

SOURCES += tst_pluginmanager.cpp

OTHER_FILES = $$PWD/plugins/otherplugin.xml \
    $$PWD/plugins/plugin1.xml \
    $$PWD/plugins/myplug/myplug.xml

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../copy.pri)
