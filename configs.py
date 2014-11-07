
'''

    @filename: configs.py
    @project : AVR32-GCC-IDE

    PhilRobotics | Philippine Electronics and Robotics Enthusiasts Club
    http://philrobotics.com | http://philrobotics.com/forum | http://facebook.com/philrobotics
    phirobotics.core@philrobotics.com

    Copyright (C) 2014  Julius Constante

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses

'''
import os, glob, re
from PyQt4 import QtCore, QtGui

# directory containing ini files
CONFIG_DIR = 'configs/'
# IDE configurations file
IDE_CONFIG = CONFIG_DIR + 'ide.ini'
# compiler configurations file
COMPILER_CONFIG = CONFIG_DIR + 'compiler.ini'
# firmware configurations file
FIRMWARE_CONFIG = CONFIG_DIR + 'firmware.ini'
# board settings file
BOARD_CONFIG = CONFIG_DIR + 'board.ini'

# default toolchains (AVR GCC)
DEFAULT_TCHAIN_WIN32 = 'tools\\avr32-gnu-toolchain\\bin\\avr32-'
DEFAULT_TCHAIN_LINUX = 'tools/avr32-gnu-toolchain/bin/avr32-'
DEFAULT_TCHAIN_OSX   = 'tools/avr32-gnu-toolchain/bin/avr32-'
# make command
DEFAULT_MAKECMD_WIN32 = 'tools\\shellUtils\\make.exe'
DEFAULT_MAKECMD_LINUX = 'make'
DEFAULT_MAKECMD_OSX   = 'make'
# rm command
DEFAULT_RMCMD_WIN32 = 'tools/shellUtils/rm -rf'
DEFAULT_RMCMD_LINUX = 'rm -rf'
DEFAULT_RMCMD_OSX   = 'rm -rf'

# compiler defines (used also for header parser)
DEFAULT_COMPILER_DEFINES = 'USE_ASF:1;BOARD:USER_BOARD'

DEFAULT_MCUARCH = 'ucr3'
DEFAULT_MCUPART = 'uc3l0128'

# default flags
COMMON_FLAGS     = '-march=$(MCUARCH) -mpart=$(MCUPART)'
DEFAULT_CFLAGS   = COMMON_FLAGS + ' -c -Os -mrelax -mno-cond-exec-before-reload'
DEFAULT_CFLAGS  += ' -fno-common -ffunction-sections -fdata-sections -funsigned-char -fno-strict-aliasing'
DEFAULT_CFLAGS  += ' -Wall -Wl,--gc-sections -Wno-psabi'
DEFAULT_CXXFLAGS = DEFAULT_CFLAGS + ' -fno-rtti -fno-exceptions'
DEFAULT_AFLAGS   = COMMON_FLAGS + ' -c -D__ASSEMBLY__ -mrelax'
DEFAULT_LFLAGS   = COMMON_FLAGS + ' -nostartfiles -T$(LKR_SCRIPT)'
DEFAULT_LFLAGS  += ' -Wl,-Map=$(MAP_FILE),--cref,--gc-sections,-e,_trampoline,--relax,--direct-data'
DEFAULT_LFLAGS  += ' -lc -lm -lgcc -lstdc++'


class IdeConfig:
    '''
    classdocs
    '''
    def __init__(self, parent=None):
        '''
        Constructor
        '''
        self.parent = parent # QMainWindow parent
        self.ideCfg = QtCore.QSettings(IDE_CONFIG, QtCore.QSettings.IniFormat, self.parent)
        self.defaults = False

    def restoreIdeSettings( self ):
        # reads settings from previous session
        self.ideCfg.beginGroup( "MainWindow" )
        self.parent.restoreGeometry(self.ideCfg.value("geometry").toByteArray());
        self.parent.restoreState(self.ideCfg.value("windowState").toByteArray());
        self.ideCfg.endGroup()

        self.ideCfg.beginGroup( "SerialPort" )
        self.serialPortName = self.ideCfg.value("Name", QtCore.QVariant('')).toString()
        self.ideCfg.endGroup()

        self.ideCfg.beginGroup( "MCUPart" )
        self.McuPartName = self.ideCfg.value("Name", QtCore.QVariant(DEFAULT_MCUPART)).toString()
        self.ideCfg.endGroup()

        #todo: other IDE settings

    def saveIdeSettings( self, serialPortName='', mcuPartName='' ):
        if self.defaults:
            return
        # save IDE settings.
        self.ideCfg.beginGroup( "MainWindow" )
        self.ideCfg.setValue("geometry", self.parent.saveGeometry());
        self.ideCfg.setValue("windowState", self.parent.saveState());
        self.ideCfg.endGroup()

        if serialPortName:
            self.ideCfg.beginGroup( "SerialPort" )
            self.ideCfg.setValue( "Name", QtCore.QVariant( serialPortName ) )
            self.serialPortName = serialPortName
            self.ideCfg.endGroup()

        if mcuPartName:
            self.ideCfg.beginGroup( "MCUPart" )
            self.ideCfg.setValue( "Name", QtCore.QVariant( mcuPartName ) )
            self.McuPartName = mcuPartName
            self.ideCfg.endGroup()

        #todo: other IDE settings

    def setDefaults(self):
        result = QtGui.QMessageBox.question(self.parent, "Restore Defaults",
                         "Continue clearing configuration (*.ini) files",
                         "OK", "Cancel")
        if result:
            return
        # backup *.ini files
        for fname in glob.glob(CONFIG_DIR + '*.ini'):
            bkpfile = fname + '.bak'
            try:
                os.remove(bkpfile) # remove first existing backup file
            except:
                pass
            try:
                os.rename(fname, bkpfile)
            except:
                pass
        QtGui.QMessageBox.about( self.parent, "Restore Defaults", "Please restart the IDE" )
        self.defaults = True

    def getSerialPortName(self):
        return self.serialPortName

    def getMcuPartName(self):
        return self.McuPartName


class CompilerConfig:
    '''
    classdocs
    '''
    def __init__(self, parent=None):
        '''
        Constructor
        '''
        self.parent = parent

        self.compilerCfg = QtCore.QSettings(COMPILER_CONFIG, QtCore.QSettings.IniFormat, self.parent)

        self.restoreCompilerSettings()

    def restoreCompilerSettings(self):
        self.compilerCfg.beginGroup("TOOLCHAIN")
        if os.sys.platform == 'win32':
            self.CC = self.compilerCfg.value("COMPILER",
                            QtCore.QVariant(DEFAULT_TCHAIN_WIN32)).toString()
        elif os.sys.platform == 'linux2':
            self.CC = self.compilerCfg.value("COMPILER",
                            QtCore.QVariant(DEFAULT_TCHAIN_LINUX)).toString()
        elif os.sys.platform == 'darwin':
            self.CC = self.compilerCfg.value("COMPILER",
                            QtCore.QVariant(DEFAULT_TCHAIN_OSX)).toString()
        else:
            # todo: other host platform
            self.CC = ""

        self.CFLAGS = self.compilerCfg.value("CFLAGS", QtCore.QVariant(DEFAULT_CFLAGS)).toString()
        self.CXXFLAGS = self.compilerCfg.value("CXXFLAGS", QtCore.QVariant(DEFAULT_CXXFLAGS)).toString()
        self.AFLAGS = self.compilerCfg.value("AFLAGS", QtCore.QVariant(DEFAULT_AFLAGS)).toString()
        self.LFLAGS = self.compilerCfg.value("LFLAGS", QtCore.QVariant(DEFAULT_LFLAGS)).toString()
        self.compilerCfg.endGroup()

        self.compilerCfg.beginGroup("MAKEFILE")
        if os.sys.platform == 'win32':
            self.MAKE = self.compilerCfg.value("MAKE",
                            QtCore.QVariant(DEFAULT_MAKECMD_WIN32)).toString()
            if str(self.MAKE)==DEFAULT_MAKECMD_WIN32:
                os.putenv('CYGWIN', 'nodosfilewarning')
            self.RM = self.compilerCfg.value("RM",
                            QtCore.QVariant(DEFAULT_RMCMD_WIN32)).toString()
        elif os.sys.platform == 'linux2':
            self.MAKE = self.compilerCfg.value("MAKE",
                            QtCore.QVariant(DEFAULT_MAKECMD_LINUX)).toString()
            self.RM = self.compilerCfg.value("RM",
                            QtCore.QVariant(DEFAULT_RMCMD_LINUX)).toString()
        elif os.sys.platform == 'darwin':
            self.MAKE = self.compilerCfg.value("MAKE",
                            QtCore.QVariant(DEFAULT_MAKECMD_OSX)).toString()
            self.RM = self.compilerCfg.value("RM",
                            QtCore.QVariant(DEFAULT_RMCMD_OSX)).toString()
        else:
            # todo: other host platform
            self.MAKE = ""
        self.compilerCfg.endGroup()

    def saveCompilerSettings(self):
        # todo: add to menu
        self.compilerCfg.beginGroup("TOOLCHAIN")
        self.compilerCfg.setValue( "COMPILER", QtCore.QVariant( self.CC ) )
        self.compilerCfg.setValue( "CFLAGS", QtCore.QVariant( self.CFLAGS ) )
        self.compilerCfg.setValue( "CXXFLAGS", QtCore.QVariant( self.CXXFLAGS ) )
        self.compilerCfg.setValue( "AFLAGS", QtCore.QVariant( self.AFLAGS ) )
        self.compilerCfg.setValue( "LFLAGS", QtCore.QVariant( self.LFLAGS ) )
        self.compilerCfg.endGroup()

        self.compilerCfg.beginGroup("MAKEFILE")
        self.compilerCfg.setValue( "MAKECMD", QtCore.QVariant( self.MAKE ) )
        self.compilerCfg.setValue( "RMCMD", QtCore.QVariant( self.RM ) )
        self.compilerCfg.endGroup()

    def getCompiler(self):
        return str( self.CC )

    def getCflags(self):
        return str( self.CFLAGS )

    def getCxxflags(self):
        return str( self.CXXFLAGS )

    def getAflags(self):
        return str( self.AFLAGS )

    def getLflags(self):
        return str( self.LFLAGS )

    def getMakeCmd(self):
        return str( self.MAKE )

    def getRmCmd(self):
        return str( self.RM )

class FirmwareConfig:
    '''
    classdocs
    '''
    def __init__(self, parent=None):
        '''
        Constructor
        '''
        self.parent = parent

        self.FwCfg = QtCore.QSettings(FIRMWARE_CONFIG, QtCore.QSettings.IniFormat, self.parent)

        self.restoreFwSettings()

    def restoreFwSettings(self):
        self.FwCfg.beginGroup("VALUES")

        self.defines = self.FwCfg.value("DEFINES", QtCore.QVariant(DEFAULT_COMPILER_DEFINES)).toString()

        self.FwCfg.endGroup()

    def saveFwSettings(self):
        self.FwCfg.beginGroup("VALUES")

        self.FwCfg.setValue( "DEFINES", QtCore.QVariant( self.defines ) )

        self.FwCfg.endGroup()

    def getDefines(self):
        define_dict = {}
        for raw in str(self.defines).split(';'):
            try:
                macro = raw.split(':')
                define_dict[macro[0]] = macro[1] # append new value
            except:
                pass
        return define_dict


# from djangoproject utils
def get_svn_revision(path=None):
    rev = None
    if path is None:
        path = os.getcwd()
    entries_path = '%s/.svn/entries' % path

    if os.path.exists(entries_path):
        entries = open(entries_path, 'r').read()
        # Versions >= 7 of the entries file are flat text.  The first line is
        # the version number. The next set of digits after 'dir' is the revision.
        if re.match('(\d+)', entries):
            rev_match = re.search('\d+\s+dir\s+(\d+)', entries)
            if rev_match:
                rev = rev_match.groups()[0]
        # Older XML versions of the file specify revision as an attribute of
        # the first entries node.
        else:
            from xml.dom import minidom
            dom = minidom.parse(entries_path)
            rev = dom.getElementsByTagName('entry')[0].getAttribute('revision')

    if rev:
        return u'svn-r%s' % rev
    return rev

