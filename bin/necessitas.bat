@SET BIN_PATH=%~dp0
@SET BIN_PATH=%BIN_PATH:\=/%
@SET PATH=%BIN_PATH%;%PATH%

@SET LD_LIBRARY_PATH=%BIN_PATH%/../Qt/lib;%LD_LIBRARY_PATH%
@SET QT_PLUGIN_PATH=%BIN_PATH%/../Qt/plugins;%QT_PLUGIN_PATH%
@SET QT_IMPORT_PATH=%BIN_PATH%/../Qt/imports;%QT_IMPORT_PATH%

start %BIN_PATH%/qtcreator.exe %*
