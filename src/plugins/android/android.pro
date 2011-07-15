TEMPLATE = lib
TARGET = Android

include(../../qtcreatorplugin.pri)
include(android_dependencies.pri)

QT += xml

HEADERS += \
    androidconstants.h \
    androidconfigurations.h \
    androidmanager.h \
    androidrunconfiguration.h \
    androidrunconfigurationwidget.h \
    androidruncontrol.h \
    androidrunfactories.h \
    androidsettingspage.h \
    androidsettingswidget.h \
    androidtoolchain.h \
    androidpackageinstallationstep.h \
    androidpackageinstallationfactory.h \
    androidpackagecreationstep.h \
    androidpackagecreationfactory.h \
    androidpackagecreationwidget.h \
    androiddeploystep.h \
    androiddeploystepwidget.h \
    androiddeploystepfactory.h \
    androidglobal.h \
    androidrunner.h \
    androiddebugsupport.h \
    androiddevicesmodel.h \
    androidqtversionfactory.h \
    androidqtversion.h \
    androiddeployconfiguration.h \
    androidtarget.h \
    androidtargetfactory.h

SOURCES += \
    androidconfigurations.cpp \
    androidmanager.cpp \
    androidrunconfiguration.cpp \
    androidrunconfigurationwidget.cpp \
    androidruncontrol.cpp \
    androidrunfactories.cpp \
    androidsettingspage.cpp \
    androidsettingswidget.cpp \
    androidtoolchain.cpp \
    androidpackageinstallationstep.cpp \
    androidpackageinstallationfactory.cpp \
    androidpackagecreationstep.cpp \
    androidpackagecreationfactory.cpp \
    androidpackagecreationwidget.cpp \
    androiddeploystep.cpp \
    androiddeploystepwidget.cpp \
    androiddeploystepfactory.cpp \
    androidrunner.cpp \
    androiddebugsupport.cpp \
    androiddevicesmodel.cpp \
    androidqtversionfactory.cpp \
    androidqtversion.cpp \
    androiddeployconfiguration.cpp \
    androidtarget.cpp \
    androidtargetfactory.cpp


FORMS += \
    androidsettingswidget.ui \
    androidpackagecreationwidget.ui \
    androiddeploystepwidget.ui \
    addnewavddialog.ui

OTHER_FILES += Android.pluginspec.in

RESOURCES = android.qrc
DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += ANDROID_LIBRARY
