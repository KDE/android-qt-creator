#!/usr/bin/env python
################################################################################
# Copyright (c) 2011 Nokia Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#   * Neither the name of Nokia Corporation, nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
################################################################################

import os
import sys
import getopt
import subprocess
import re
import string
import shutil
from glob import glob

ignoreErrors = False
debug_build = False

def usage():
    print "Usage: %s <creator_install_dir> [qmake_path]" % os.path.basename(sys.argv[0])

def which(program):
    def is_exe(fpath):
        return os.path.exists(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
        if sys.platform.startswith('win'):
            if is_exe(program + ".exe"):
                return program  + ".exe"
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
            if sys.platform.startswith('win'):
                if is_exe(exe_file + ".exe"):
                    return exe_file  + ".exe"

    return None

def is_debug(fpath):
    # bootstrap exception
    if fpath.endswith('QtCore4d.dll'):
        return True
    output = subprocess.check_output(['dumpbin', '/imports', fpath])
    return output.find('QtCored4.dll') != -1

def is_debug_build(install_dir):
    return is_debug(os.path.join(install_dir, 'bin', 'qtcreator.exe'))

def op_failed(details = None):
    if details != None:
        print details
    if ignoreErrors == False:
        print("Error: operation failed!")
        sys.exit(2)
    else:
        print("Error: operation failed, but proceeding gracefully.")

def fix_rpaths_helper(chrpath_bin, install_dir, dirpath, filenames):
    # patch file
    for filename in filenames:
        fpath = os.path.join(dirpath, filename)
        relpath = os.path.relpath(install_dir+'/lib/qtcreator', dirpath)
        command = [chrpath_bin, '-r', '$ORIGIN/'+relpath, fpath]
        print fpath, ':', command
        try:
            subprocess.check_call(command)
        except:
            op_failed()

def check_unix_binary_exec_helper(dirpath, filename):
    """ Whether a file is really a binary executable and not a script (unix only)"""
    fpath = os.path.join(dirpath, filename)
    if os.path.exists(fpath) and os.access(fpath, os.X_OK):
        with open(fpath) as f:
            return f.read(2) != "#!"

def check_unix_library_helper(dirpath, filename):
    """ Whether a file is really a library and not a symlink (unix only)"""
    fpath = os.path.join(dirpath, filename)
    return filename.find('.so') != -1 and not os.path.islink(fpath)

def fix_rpaths(chrpath_bin, install_dir):
    print "fixing rpaths..."
    for dirpath, dirnames, filenames in os.walk(os.path.join(install_dir, 'bin')):
        #TODO remove library_helper once all libs moved out of bin/ on linux
        filenames = [filename for filename in filenames if check_unix_binary_exec_helper(dirpath, filename) or check_unix_library_helper(dirpath, filename)]
        fix_rpaths_helper(chrpath_bin, install_dir, dirpath, filenames)
    for dirpath, dirnames, filenames in os.walk(os.path.join(install_dir, 'lib')):
        filenames = [filename for filename in filenames if check_unix_library_helper(dirpath, filename)]
        fix_rpaths_helper(chrpath_bin, install_dir, dirpath, filenames)

def windows_debug_files_filter(filename):
    ignore_patterns = ['.lib', '.pdb', '.exp', '.ilk']
    for ip in ignore_patterns:
        if filename.endswith(ip):
            return True
    return False

def copy_ignore_patterns_helper(dir, filenames):
    if not sys.platform.startswith('win'):
        return filenames

    if debug_build:
        wrong_dlls = filter(lambda filename: filename.endswith('.dll') and not is_debug(os.path.join(dir, filename)), filenames)
    else:
        wrong_dlls = filter(lambda filename: filename.endswith('.dll') and is_debug(os.path.join(dir, filename)), filenames)

    filenames = wrong_dlls + filter(windows_debug_files_filter, filenames)
    return filenames

def copy_qt_libs(install_dir, qt_libs_dir, qt_plugin_dir, qt_import_dir, plugins, imports):
    print "copying Qt libraries..."

    if sys.platform.startswith('win'):
        libraries = glob(os.path.join(qt_libs_dir, '*.dll'))
    else:
        libraries = glob(os.path.join(qt_libs_dir, '*.so.*'))

    if sys.platform.startswith('win'):
        dest = os.path.join(install_dir, 'bin')
    else:
        dest = os.path.join(install_dir, 'lib', 'qtcreator')

    if sys.platform.startswith('win'):
        if debug_build:
            libraries = filter(lambda library: is_debug(library), libraries)
        else:
            libraries = filter(lambda library: not is_debug(library), libraries)

    for library in libraries:
        print library, '->', dest
        if os.path.islink(library):
            linkto = os.readlink(library)
            try:
                os.symlink(linkto, os.path.join(dest, os.path.basename(library)))
            except:
                op_failed("Link already exists!")
        else:
            shutil.copy(library, dest)

    copy_ignore_func = None
    if sys.platform.startswith('win'):
        copy_ignore_func = copy_ignore_patterns_helper

    print "Copying plugins:", plugins
    for plugin in plugins:
        target = os.path.join(install_dir, 'bin', plugin)
        if (os.path.exists(target)):
            shutil.rmtree(target)
        shutil.copytree(os.path.join(qt_plugin_dir, plugin), target, ignore=copy_ignore_func, symlinks=True)

    print "Copying plugins:", imports
    for qtimport in imports:
        target = os.path.join(install_dir, 'bin', qtimport)
        if (os.path.exists(target)):
            shutil.rmtree(target)
        shutil.copytree(os.path.join(qt_import_dir, qtimport), target, ignore=copy_ignore_func, symlinks=True)

def copy_translations(install_dir, qt_tr_dir, tr_catalogs):
    langs = []
    tr_dir = os.path.join(install_dir, 'share', 'qtcreator', 'translations')
    p = re.compile(r'_(.*).qm')
    for dirpath, dirnames, filenames in os.walk(tr_dir):
        for filename in filenames:
            if filename.endswith('.qm') and string.find(filename, 'qtcreator_') != -1:
                lang = p.findall(filename)
                if lang != '':
                    langs += lang

    print "copying translations..."
    for lang in langs:
        for catalog in tr_catalogs:
            copy_file = "%s_%s.qm" % (catalog, lang)
            copy_src = os.path.join(qt_tr_dir, copy_file)
            copy_dst = os.path.join(tr_dir, copy_file)
            print copy_src, '->', copy_dst
            shutil.copy(copy_src, copy_dst)

def readQmakeVar(qmake_bin, var):
    pipe = os.popen(' '.join([qmake_bin, '-query', var]))
    return pipe.read().rstrip('\n')

def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], 'hi', ['help', 'ignore-errors'])
    except:
        usage()
        sys.exit(2)
    for o, a in opts:
        if o in ('-h', '--help'):
            usage()
            sys.exit(0)
        if o in ('-i', '--ignore-errors'):
            global ignoreErrors
            ignoreErrors = True
            print "Note: Ignoring all errors"

    if len(args) < 1:
        usage()
        sys.exit(2)

    install_dir = args[0]

    qmake_bin = 'qmake'
    if len(args) > 1:
        qmake_bin = args[1]
    qmake_bin = which(qmake_bin)

    if qmake_bin == None:
        print "Cannot find required binary 'qmake'."
        sys.exit(2)

    if not sys.platform.startswith('win'):
        chrpath_bin = which('chrpath')
        if chrpath_bin == None:
            print "Cannot find required binary 'chrpath'."
            sys.exit(2)

    QT_INSTALL_LIBS = readQmakeVar(qmake_bin, 'QT_INSTALL_LIBS')
    QT_INSTALL_BINS = readQmakeVar(qmake_bin, 'QT_INSTALL_BINS')
    QT_INSTALL_PLUGINS = readQmakeVar(qmake_bin, 'QT_INSTALL_PLUGINS')
    QT_INSTALL_IMPORTS = readQmakeVar(qmake_bin, 'QT_INSTALL_IMPORTS')
    QT_INSTALL_TRANSLATIONS = readQmakeVar(qmake_bin, 'QT_INSTALL_TRANSLATIONS')

    plugins = ['accessible', 'designer', 'iconengines', 'imageformats', 'sqldrivers']
    imports = ['Qt', 'QtWebKit']
    tr_catalogs = ['assistant', 'designer', 'qt', 'qt_help']

    if sys.platform.startswith('win'):
        global debug_build
        debug_build = is_debug_build(install_dir)

    if sys.platform.startswith('win'):
      copy_qt_libs(install_dir, QT_INSTALL_BINS, QT_INSTALL_PLUGINS, QT_INSTALL_IMPORTS, plugins, imports)
    else:
      copy_qt_libs(install_dir, QT_INSTALL_LIBS, QT_INSTALL_PLUGINS, QT_INSTALL_IMPORTS, plugins, imports)
    copy_translations(install_dir, QT_INSTALL_TRANSLATIONS, tr_catalogs)

    if not sys.platform.startswith('win'):
        fix_rpaths(chrpath_bin, install_dir)

if __name__ == "__main__":
    if sys.platform == 'darwin':
        print "Mac OS is not supported by this script, please use macqtdeploy!"
        sys.exit(2)
    else:
        main()
