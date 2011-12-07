# for easier re-usage (because Python hasn't an enum type)
class QtQuickConstants:
    class Components:
        BUILTIN = 1
        SYMBIAN = 2
        MEEGO_HARMATTAN = 4
        EXISTING_QML = 8

    class Targets:
        DESKTOP = 1
        SIMULATOR = 2
        SYMBIAN = 4
        MAEMO5 = 8
        HARMATTAN = 16

    @staticmethod
    def getStringForComponents(components):
            if components==QtQuickConstants.Components.BUILTIN:
                return "Built-in elements only (for all platforms)"
            elif components==QtQuickConstants.Components.SYMBIAN:
                return "Qt Quick Components for Symbian"
            elif components==QtQuickConstants.Components.MEEGO_HARMATTAN:
                return "Qt Quick Components for Meego/Harmattan"
            elif components==QtQuickConstants.Components.EXISTING_QML:
                return "Use an existing .qml file"
            else:
                return None

    @staticmethod
    def getStringForTarget(target):
        if target==QtQuickConstants.Targets.DESKTOP:
            return "Desktop"
        elif target==QtQuickConstants.Targets.SYMBIAN:
            return "Symbian Device"
        elif target==QtQuickConstants.Targets.MAEMO5:
            return "Maemo5"
        elif target==QtQuickConstants.Targets.SIMULATOR:
            return "Qt Simulator"
        elif target==QtQuickConstants.Targets.HARMATTAN:
            return "Harmattan"
        else:
            return None

# this class holds some constants for easier usage inside the Projects view
class ProjectSettings:
    BUILD = 1
    RUN = 2

# this class defines some constants for the views of the creator's MainWindow
class ViewConstants:
    WELCOME = 0
    EDIT = 1
    DESIGN = 2
    DEBUG = 3
    PROJECTS = 4
    ANALYZE = 5
    HELP = 6
    # always adjust the following to the highest value of the available ViewConstants when adding new
    LAST_AVAILABLE = HELP

    # this function returns a regex of the tooltip of the FancyTabBar elements
    # this is needed because the keyboard shortcut is OS specific
    # if the provided argument does not match any of the ViewConstants it returns None
    @staticmethod
    def getToolTipForViewTab(viewTab):
        if viewTab == ViewConstants.WELCOME:
            return ur'Switch to <b>Welcome</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)1</span>'
        elif viewTab == ViewConstants.EDIT:
            return ur'Switch to <b>Edit</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)2</span>'
        elif viewTab == ViewConstants.DESIGN:
            return ur'Switch to <b>Design</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)3</span>'
        elif viewTab == ViewConstants.DEBUG:
            return ur'Switch to <b>Debug</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)4</span>'
        elif viewTab == ViewConstants.PROJECTS:
            return ur'Switch to <b>Projects</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)5</span>'
        elif viewTab == ViewConstants.ANALYZE:
            return ur'Switch to <b>Analyze</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)6</span>'
        elif viewTab == ViewConstants.HELP:
            return ur'Switch to <b>Help</b> mode <span style="color: gray; font-size: small">(Ctrl\+|\u2303)7</span>'
        else:
            return None

class SubprocessType:
    QT_WIDGET=0
    QT_QUICK_APPLICATION=1
    QT_QUICK_UI=2
    USER_DEFINED=3

    @staticmethod
    def getWindowType(subprocessType):
        if subprocessType == SubprocessType.QT_WIDGET:
            return "QMainWindow"
        if subprocessType == SubprocessType.QT_QUICK_APPLICATION:
            return "QmlApplicationViewer"
        if subprocessType == SubprocessType.QT_QUICK_UI:
            return "QDeclarativeViewer"
        if subprocessType == SubprocessType.USER_DEFINED:
            return "user-defined"
        test.fatal("Could not determine the WindowType for SubprocessType %s" % subprocessType)
        return None

