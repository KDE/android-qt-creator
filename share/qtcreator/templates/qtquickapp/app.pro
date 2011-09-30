# Add more folders to ship with the application, here
# DEPLOYMENTFOLDERS #
folder_01.source = qml/app
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01
# DEPLOYMENTFOLDERS_END #

# Additional import path used to resolve QML modules in Creator's code model
# QML_IMPORT_PATH #
QML_IMPORT_PATH =

# TARGETUID3 #
symbian:TARGET.UID3 = 0xE1111234

# Smart Installer package's UID
# This UID is from the protected range and therefore the package will
# fail to install if self-signed. By default qmake uses the unprotected
# range value if unprotected UID is defined for the application and
# 0x2002CCCF value if protected UID is given to the application
#symbian:DEPLOYMENT.installer_header = 0x2002CCCF

# Allow network access on Symbian
# NETWORKACCESS #
symbian:TARGET.CAPABILITY += NetworkServices

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

# Speed up launching on MeeGo/Harmattan when using applauncherd daemon
# HARMATTAN_BOOSTABLE #
# CONFIG += qdeclarative-boostable

# Add dependency to Symbian components
# QTQUICKCOMPONENTS #
# CONFIG += qt-components

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp

# Please do not modify the following two lines. Required for deployment.
include(qmlapplicationviewer/qmlapplicationviewer.pri)
# REMOVE_NEXT_LINE (wizard will remove the include and append deployment.pri to qmlapplicationviewer.pri, instead) #
include(../shared/deployment.pri)
qtcAddDeployment()
