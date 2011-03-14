#!/bin/bash

echo "$0 :: Usage <dest-dir> <pre-existing-offical-nokia-qt-creator-install-dir> <pre-existing-mingw-qt-install-dir>"
echo "e.g. $0 C:/Necessitas/qtcreator-2.1.0 C:/Qt/qtcreator-2.1.0 C:/Qt/4.7.2-Git-MinGW-ShXcR"

if [ "$1" != "" ] ; then
	DESTQTDIR=$1
fi

if [ "$2" != "" ] ; then
	OFFICIALNOKIAQTC210INST=$2
fi

# This *must* be the same Qt that QtCreator was built with.
if [ "$3" != "" ] ; then
	MINGWQTC210INST=$3
fi

if [ "$DESTQTDIR" = "" ] ; then
	DESTQTDIR=$QTDIR
fi

rm -rf $DESTQTDIR

SRCDIR=`dirname $0`

PYTHONDIR="$SRCDIR/../../mingw-python/Python-2.7.1-mingw"
# NDKDIR="/usr/android-sdk-windows/android-ndk-r5b"

echo "Copying QtCreator from $SRCDIR to $DESTQTDIR,"
echo ":: Using additional directories of ::"
echo "Nokia QtCreator Install for license files         :: $OFFICIALNOKIAQTC210INST"
echo "MinGW Qt 4.7.2 Build for dlls                     :: $MINGWQTC210INST"
echo "Python directory for LICENSE-PYTHON               :: $PYTHONDIR"
# echo "NDK directory for mingw.android gdb and gdbserver :: $NDKDIR"

mkdir -p $DESTQTDIR/lib/qtcreator/plugins/Nokia
mkdir -p $DESTQTDIR/bin
mkdir -p $DESTQTDIR/share
mkdir -p $DESTQTDIR/share/pixmaps
mkdir -p $DESTQTDIR/pythongdb
mkdir -p $DESTQTDIR/pythongdb/ma-mingw-gdb
mkdir -p $DESTQTDIR/pythongdb/ma-android-gdb

# Copy our new binaries libs and plugins.
cp -f bin/* $DESTQTDIR/bin/
cp -f lib/qtcreator/*.dll $DESTQTDIR/lib/qtcreator/
cp -f lib/qtcreator/plugins/Nokia/* $DESTQTDIR/lib/qtcreator/plugins/Nokia/
cp -rf share/* $DESTQTDIR/share/

# Copy some other stuff, docs and licenses.
cp -rf $SRCDIR/doc $DESTQTDIR/share/
cp -f $SRCDIR/src/plugins/coreplugin/images/qtcreator_logo*.png $DESTQTDIR/share/pixmaps/
cp -f $SRCDIR/*LICENSE* $DESTQTDIR/
cp -f $SRCDIR/README $DESTQTDIR/
cp -f $PYTHONDIR/LICENSE $DESTQTDIR/LICENSE-PYTHON

# Oops, these are generated from within QtCreator.
# if [ "$OFFICIALNOKIAQTC210INST" != "" ] ; then
#	cp -rf $OFFICIALNOKIAQTC210INST/pythongdb/* $DESTQTDIR/pythongdb/
# fi

# Grab my newer gdbs from Google code instead, MinGW and Android put in separate folders.
wget -c http://mingw-and-ndk.googlecode.com/files/gdb-7.2.50.20110211-mingw32-bin.7z
mkdir mingwgdbtemp
wget -c http://mingw-and-ndk.googlecode.com/files/android-ndk-toolchain-4.4.3-gdb-7.2.50-python-2.7.1-7.2.50.20110211-windows.7z
mkdir andgdbtemp

"C:/Program Files/7-zip/7z.exe" x -y -omingwgdbtemp gdb-7.2.50.20110211-mingw32-bin.7z
mv -f mingwgdbtemp/* $DESTQTDIR/pythongdb/gdb-ma-mingw/
rmdir mingwgdbtemp

"C:/Program Files/7-zip/7z.exe" x -y -oandgdbtemp android-ndk-toolchain-4.4.3-gdb-7.2.50-python-2.7.1-7.2.50.20110211-windows.7z
mv -f mingwandtemp/* $DESTQTDIR/pythongdb/gdb-ma-android/
rmdir mingwandtemp

cp -f $SRCDIR/../mingw-android-lighthouse/qpatch.exe $DESTQTDIR/bin/
cp -f $SRCDIR/../mingw-android-lighthouse/files-to-patch-* $DESTQTDIR/bin/

# cp -f $NDKDIR/toolchains/arm-linux-androideabi-4.4.3/prebuilt/windows/bin/arm-linux-androideabi-gdb*.exe $DESTQTDIR/pythongdb/
# cp -f $NDKDIR/toolchains/arm-linux-androideabi-4.4.3/prebuilt/windows/bin/*.dll $DESTQTDIR/pythongdb/
cp -f $SRCDIR/mingw/*.dll $DESTQTDIR/bin/

if [ "$OFFICIALNOKIAQTC210INST" != "" ] ; then
	cp -f $OFFICIALNOKIAQTC210INST/share/qtcreator/LGPL_EXCEPTION.TXT $DESTQTDIR/share/qtcreator
	cp -f $OFFICIALNOKIAQTC210INST/share/qtcreator/LICENSE.LGPL $DESTQTDIR/share/qtcreator
fi

# Copy stuff over to allow Necessitas to work for MinGW too.
# Happily, these don't conflict with the Android .so files.
cp -f $MINGWQTC210INST/bin/*.dll $DESTQTDIR/bin/
echo "This will report:"
echo "cp: omitting directory $MINGWQTC210INST/lib/qtcreator ... just ignore it, we don't want that folder anyway."
cp -f $MINGWQTC210INST/lib/* $DESTQTDIR/lib/
