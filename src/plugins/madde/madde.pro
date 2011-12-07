TEMPLATE = lib
TARGET = Madde

include(../../qtcreatorplugin.pri)
include(madde_dependencies.pri)

HEADERS += \
    madde_exports.h \
    maddeplugin.h \
    maemoconstants.h \
    maemorunconfigurationwidget.h \
    maemoruncontrol.h \
    maemorunfactories.h \
    maemosettingspages.h \
    maemotoolchain.h \
    maemopackagecreationstep.h \
    maemopackagecreationfactory.h \
    maemopackagecreationwidget.h \
    maemoqemumanager.h \
    maemodeploystepfactory.h \
    maemoglobal.h \
    maemosshrunner.h \
    maemodebugsupport.h \
    maemoremotemountsmodel.h \
    maemomountspecification.h \
    maemoremotemounter.h \
    maemopublishingwizardfactories.h \
    maemopublishingbuildsettingspagefremantlefree.h \
    maemopublishingfileselectiondialog.h \
    maemopublishedprojectmodel.h \
    maemopublishinguploadsettingspagefremantlefree.h \
    maemopublishingwizardfremantlefree.h \
    maemopublishingresultpagefremantlefree.h \
    maemopublisherfremantlefree.h \
    maemoqemuruntime.h \
    maemoqemuruntimeparser.h \
    maemoqemusettingswidget.h \
    maemoqemusettings.h \
    qt4maemotargetfactory.h \
    qt4maemotarget.h \
    qt4maemodeployconfiguration.h \
    maemodeviceconfigwizard.h \
    maemodeployconfigurationwidget.h \
    maemoinstalltosysrootstep.h \
    maemodeploymentmounter.h \
    maemopackageinstaller.h \
    maemoremotecopyfacility.h \
    maemoqtversionfactory.h \
    maemoqtversion.h \
    maemorunconfiguration.h \
    maddeuploadandinstallpackagesteps.h \
    maemodeploybymountsteps.h \
    maddedevicetester.h \
    maddedeviceconfigurationfactory.h \

SOURCES += \
    maddeplugin.cpp \
    maemorunconfigurationwidget.cpp \
    maemoruncontrol.cpp \
    maemorunfactories.cpp \
    maemosettingspages.cpp \
    maemotoolchain.cpp \
    maemopackagecreationstep.cpp \
    maemopackagecreationfactory.cpp \
    maemopackagecreationwidget.cpp \
    maemoqemumanager.cpp \
    maemodeploystepfactory.cpp \
    maemoglobal.cpp \
    maemosshrunner.cpp \
    maemodebugsupport.cpp \
    maemoremotemountsmodel.cpp \
    maemomountspecification.cpp \
    maemoremotemounter.cpp \
    maemopublishingwizardfactories.cpp \
    maemopublishingbuildsettingspagefremantlefree.cpp \
    maemopublishingfileselectiondialog.cpp \
    maemopublishedprojectmodel.cpp \
    maemopublishinguploadsettingspagefremantlefree.cpp \
    maemopublishingwizardfremantlefree.cpp \
    maemopublishingresultpagefremantlefree.cpp \
    maemopublisherfremantlefree.cpp \
    maemoqemuruntimeparser.cpp \
    maemoqemusettingswidget.cpp \
    maemoqemusettings.cpp \
    qt4maemotargetfactory.cpp \
    qt4maemotarget.cpp \
    qt4maemodeployconfiguration.cpp \
    maemodeviceconfigwizard.cpp \
    maemodeployconfigurationwidget.cpp \
    maemoinstalltosysrootstep.cpp \
    maemodeploymentmounter.cpp \
    maemopackageinstaller.cpp \
    maemoremotecopyfacility.cpp \
    maemoqtversionfactory.cpp \
    maemoqtversion.cpp \
    maddedeviceconfigurationfactory.cpp \
    maddeuploadandinstallpackagesteps.cpp \
    maemodeploybymountsteps.cpp \
    maddedevicetester.cpp \
    maemorunconfiguration.cpp

FORMS += \
    maemopackagecreationwidget.ui \
    maemopublishingbuildsettingspagefremantlefree.ui \
    maemopublishingfileselectiondialog.ui \
    maemopublishinguploadsettingspagefremantlefree.ui \
    maemopublishingresultpagefremantlefree.ui \
    maemoqemusettingswidget.ui \
    maemodeviceconfigwizardstartpage.ui \
    maemodeviceconfigwizardpreviouskeysetupcheckpage.ui \
    maemodeviceconfigwizardreusekeyscheckpage.ui \
    maemodeviceconfigwizardkeycreationpage.ui \
    maemodeviceconfigwizardkeydeploymentpage.ui \
    maemodeployconfigurationwidget.ui

RESOURCES += qt-maemo.qrc
DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += MADDE_LIBRARY
