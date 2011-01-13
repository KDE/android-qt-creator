#!/usr/bin/perl -w

# *************************************************************************
#
# This file is part of Qt Creator
#
# Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
#
# Contact: Nokia Corporation (qt-info@nokia.com)
#
# Commercial Usage
#
# Licensees holding valid Qt Commercial licenses may use this file in
# accordance with the Qt Commercial License Agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and Nokia.
#
# GNU Lesser General Public License Usage
#
# Alternatively, this file may be used under the terms of the GNU Lesser
# General Public License version 2.1 as published by the Free Software
# Foundation and appearing in the file LICENSE.LGPL included in the
# packaging of this file.  Please review the following information to
# ensure the GNU Lesser General Public License version 2.1 requirements
# will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
#
# If you are unsure which license is appropriate for your use, please
# contact the sales department at http://qt.nokia.com/contact.
#
# *************************************************************************

use strict;
use Getopt::Long;
use IO::File;

my $optDryRun = 0;
my $optHelp = 0;
my $optClassName = 'MyClass';
my $optProjectName = 'MyProject';
my $optCppHeaderSuffix = 'h';
my $optCppSourceSuffix = 'cpp';
my $optDescription = '';

# -- Read in a file and return its lines
sub readFile
{
    my ($fileName) = @_;
    my @rc = ();
    my $fh = new IO::File('<' . $fileName) or die ('Unable to open for reading ' .  $fileName . ' :' . $!);
    while (my $line = <$fh>) {
	chomp($line);
        push (@rc, $line);
    }
    $fh->close();
    return @rc;
}

my $USAGE=<<EOF;
Usage: generate.pl [--help] | [--dry-run]
                   [--class-name=<class name>]
                   [--project-name=<project name>]
                   [--header-suffix=<header suffix>]
                   [--source-suffix=<source suffix>]
                   [--description=<description-file>]

Custom wizard project generation example script.

EOF

my $argCount = scalar(@ARGV);
if ($argCount == 0
    || !GetOptions("help" => \$optHelp,
                   "dry-run" => \$optDryRun,
                   "class-name:s" => \$optClassName,
                   "project-name:s" => \$optProjectName,
                   "header-suffix:s" => \$optCppHeaderSuffix,
                   "source-suffix:s" => \$optCppSourceSuffix,
                   "description:s" => \$optDescription)
    || $optHelp != 0) {
    print $USAGE;
    exit (1);
}

# -- Determine file names
my $baseFileName = lc($optClassName);
my $sourceFileName = $baseFileName . '.' . $optCppSourceSuffix;
my $headerFileName = $baseFileName . '.' . $optCppHeaderSuffix;
my $mainSourceFileName = 'main.' . $optCppSourceSuffix;
my $projectFileName = lc($optProjectName) . '.pro';

if ($optDryRun) {
#   -- Step 1) Dry run: Print file names along with attributes
    print $sourceFileName,",openeditor\n";
    print $headerFileName,",openeditor\n";
    print $mainSourceFileName,",openeditor\n";
    print $projectFileName,",openproject\n";
} else {
#   -- Step 2) Actual file creation
    print 'Generating ',  $headerFileName, ' ', $sourceFileName, ' ',
          $mainSourceFileName, ' ', $projectFileName, "\n";
    my $headerFile = new IO::File('>' . $headerFileName) or die ('Unable to open ' . $headerFileName . ' :' . $!);
    print $headerFile '#ifndef ', uc($optClassName), "_H\n#define ", uc($optClassName), "_H\n\n",
          'class ', $optClassName, "{\npublic:\n    ", $optClassName, "();\n\n};\n\n#endif\n";
    $headerFile->close();

    my $sourceFile = new IO::File('>' . $sourceFileName) or die ('Unable to open ' . $sourceFileName . ' :' . $!);
    print $sourceFile  '#include "', $headerFileName ,"\"\n\n",
            $optClassName,'::', $optClassName, "()\n{\n}\n";
    $sourceFile->close();

    my $mainSourceFile = new IO::File('>' . $mainSourceFileName) or die ('Unable to open ' . $mainSourceFileName . ' :' . $!);
    print $mainSourceFile  '#include "', $headerFileName ,"\"\n\n";
#   -- Write out description comments
    if ($optDescription ne '') {
        foreach my $description (readFile($optDescription)) {
	    print $mainSourceFile  '// ', $description, "\n";
	}
    }
    print $mainSourceFile "int main(int argc, char *argv[])\n{\n    ", $optClassName,' ', lc($optClassName),
          ";\n    return 0;\n}\n";
    $mainSourceFile->close();

    my $projectFile = new IO::File('>' . $projectFileName) or die ('Unable to open ' . $projectFileName . ' :' . $!);
    print $projectFile "TEMPLATE = app\nQT -= core\nCONFIG += console\nTARGET = ", $optProjectName,
          "\nSOURCES += ", $sourceFileName, ' ',$headerFileName, ' ', $mainSourceFileName,
          "\nHEADERS += ", $headerFileName,"\n";
    $projectFile->close();
}
