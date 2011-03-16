#!/bin/bash

# ../mingw-android-qt-creator/copy-to-qt-dir.sh C:/Necessitas/qtcreator-2.1.0 C:/Qt/qtcreator-2.1.0 C:/Qt/4.7.2-Git-MinGW-ShXcR C:/Qt/4.7.2-official

echo "$0 :: Usage
			<dest-dir> \n\
			<official-qt-creator-dir>\n\
			<mingw-qt-used-to-build-android-qt-creator-dir>\n\
			<pre-existing-offical-nokia-qt-dir>"

echo "e.g. $0 C:/Necessitas/qtcreator-2.1.0 C:/Qt/qtcreator-2.1.0 C:/Qt/4.7.2-Git-MinGW-ShXcR C:/Qt/4.7.2-official"

if [ "$1" != "" ] ; then
	DESTQTCDIR=$1
fi

if [ "$2" != "" ] ; then
	OFFICIALQTCDIR=$2
fi

# This *must* be the same Qt that QtCreator was built with.
if [ "$3" != "" ] ; then
	MINGWQTDIR=$3
fi

if [ "$4" != "" ] ; then
	OFFICIALQTDIR=$3
fi

if [ "$DESTQTCDIR" = "" ] ; then
	DESTQTCDIR=$QTDIR
fi

rm -rf $DESTQTCDIR

SRCDIR=`dirname $0`

PYTHONDIR="$SRCDIR/../../mingw-python"
# NDKDIR="/usr/android-sdk-windows/android-ndk-r5b"

echo "Copying QtCreator from $SRCDIR to $DESTQTCDIR,"
echo ":: Using additional directories of ::"
echo "Nokia QtCreator Install for license files           :: $OFFICIALQTCDIR"
echo "MinGW Qt 4.7.2 Install for dlls                     :: $MINGWQTDIR"
echo "Python directory for LICENSE-PYTHON                 :: $PYTHONDIR"
# echo "NDK directory for mingw.android gdb and gdbserver :: $NDKDIR"

mkdir -p $DESTQTCDIR/lib/qtcreator/plugins/Nokia
mkdir -p $DESTQTCDIR/bin
mkdir -p $DESTQTCDIR/share
mkdir -p $DESTQTCDIR/share/pixmaps
mkdir -p $DESTQTCDIR/pythongdb
mkdir -p $DESTQTCDIR/pythongdb/gdb-ma-mingw
mkdir -p $DESTQTCDIR/pythongdb/gdb-ma-android/bin

# Copy our new binaries libs and plugins.
cp -f bin/* $DESTQTCDIR/bin/
cp -f lib/qtcreator/*.dll $DESTQTCDIR/lib/qtcreator/
cp -f lib/qtcreator/plugins/Nokia/* $DESTQTCDIR/lib/qtcreator/plugins/Nokia/
cp -rf share/* $DESTQTCDIR/share/

# Copy some other stuff, docs and licenses.
cp -rf $SRCDIR/doc $DESTQTCDIR/share/
cp -f $SRCDIR/src/plugins/coreplugin/images/qtcreator_logo*.png $DESTQTCDIR/share/pixmaps/
cp -f $SRCDIR/*LICENSE* $DESTQTCDIR/
cp -f $SRCDIR/README $DESTQTCDIR/
cp -f $PYTHONDIR/LICENSE $DESTQTCDIR/LICENSE-PYTHON

# Oops, these are generated from within QtCreator.
if [ "$OFFICIALQTCDIR" != "" ] ; then
	cp -rf $OFFICIALQTCDIR/pythongdb/* $DESTQTCDIR/pythongdb/
fi

# Grab my newer gdbs from Google code instead, MinGW and Android put in separate folders.
wget -c http://mingw-and-ndk.googlecode.com/files/gdb-7.2.50.20110211-mingw32-bin.7z
mkdir mingwgdbtemp
wget -c http://mingw-and-ndk.googlecode.com/files/android-ndk-toolchain-4.4.3-gdb-7.2.50-python-2.7.1-7.2.50.20110211-windows.7z
mkdir andgdbtemp

"C:/Program Files/7-zip/7z.exe" x -y -omingwgdbtemp gdb-7.2.50.20110211-mingw32-bin.7z
mv -f mingwgdbtemp/* $DESTQTCDIR/pythongdb/gdb-ma-mingw/
rmdir mingwgdbtemp

"C:/Program Files/7-zip/7z.exe" x -y -oandgdbtemp android-ndk-toolchain-4.4.3-gdb-7.2.50-python-2.7.1-7.2.50.20110211-windows.7z

tar xvjf andgdbtemp/arm-linux-androideabi-4.4.3-gdbserver.tar.bz2

tar xvjf andgdbtemp/arm-linux-androideabi-4.4.3-windows.tar.bz2
mv toolchains/arm-linux-androideabi-4.4.3/prebuilt/windows/bin/arm-linux-androideabi-gdb.exe $DESTQTCDIR/pythongdb/gdb-ma-android/bin
mv toolchains/arm-linux-androideabi-4.4.3/prebuilt/windows/bin/arm-linux-androideabi-gdbtui.exe $DESTQTCDIR/pythongdb/gdb-ma-android/bin
mv toolchains/arm-linux-androideabi-4.4.3/prebuilt/gdbserver $DESTQTCDIR/pythongdb/gdb-ma-android/

cp -f $SRCDIR/../mingw-android-lighthouse/qpatch.exe $DESTQTCDIR/bin/
cp -f $SRCDIR/../mingw-android-lighthouse/files-to-patch-* $DESTQTCDIR/bin/

# cp -f $NDKDIR/toolchains/arm-linux-androideabi-4.4.3/prebuilt/windows/bin/arm-linux-androideabi-gdb*.exe $DESTQTCDIR/pythongdb/
# cp -f $NDKDIR/toolchains/arm-linux-androideabi-4.4.3/prebuilt/windows/bin/*.dll $DESTQTCDIR/pythongdb/
cp -f $SRCDIR/mingw/*.dll $DESTQTCDIR/bin/

if [ "$OFFICIALQTCDIR" != "" ] ; then
	cp -f $OFFICIALQTCDIR/share/qtcreator/LGPL_EXCEPTION.TXT $DESTQTCDIR/share/qtcreator
	cp -f $OFFICIALQTCDIR/share/qtcreator/LICENSE.LGPL $DESTQTCDIR/share/qtcreator
fi

# Copy stuff over to allow Necessitas to work for MinGW too.
# Happily, these don't conflict with the Android .so files.
# You end up with a choice of
cp -f $MINGWQTDIR/bin/*.dll $DESTQTCDIR/bin/
echo "This will report:"
echo "cp: omitting directory $OFFICIALQTCDIR/lib/qtcreator ... just ignore it, we don't want that folder anyway."
cp -f $OFFICIALQTCDIR/lib/* $DESTQTCDIR/lib/
