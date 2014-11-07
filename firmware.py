
'''

    @filename: firmware.py
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

import os, glob, shutil, urllib2
import clang.cindex as clang
from configs import FirmwareConfig
from PyQt4 import QtCore

# Core firmware Library
CORE_LIB_DIR = 'hardware/cores'
# User library path
USER_LIB_DIR = 'libraries'

BSP_DIR = CORE_LIB_DIR + '/bsp'
ASF_DIR = CORE_LIB_DIR + '/ASF'
DRV_DIR = ASF_DIR + '/avr32/drivers'

# Example Projects
EXAMPLES_DIR = 'examples'
# required header file
REQUIRED_INCLUDE = '#include <avr32.h>'
# user project/source code extension name
USER_CODE_EXT = '.cxx'

fwconfig = FirmwareConfig()

def scanFirmwareLibs():
    libraries = []
    folders = glob.glob(USER_LIB_DIR + '/*') # scan all folders inside LIB_DIR
    for folder in folders:
        libname = folder[len(USER_LIB_DIR)+1:]
        headerfile = folder + '/' + libname + '.h' # header filename must be based on its folder name
        if os.path.isfile(headerfile): # check if header file exists
            libraries.append( libname )
    return libraries

def getExampleProjects(libFolders=[]):
    sampleProjects = {} # use dictionary
    # get example projects that use core libraries
    sampleFolders = os.walk(EXAMPLES_DIR).next()[1] # get intermediate subfolders
    for lib in sampleFolders:
        if lib[0] != '.': # skip hidden folders
            projects = glob.glob(EXAMPLES_DIR + '/' + lib + '/*' + USER_CODE_EXT )
            if len(projects):
                group = str(lib).upper()
                if not sampleProjects.has_key(group): # avoid duplicates
                    sampleProjects[group] = projects # store in the dictionary
    # get example projects that use optional user libraries
    for lib in libFolders:
        projects = glob.glob(USER_LIB_DIR + '/' + lib + '/examples/*' + USER_CODE_EXT )
        if len(projects):
            group = str(lib).upper()
            if not sampleProjects.has_key(group):
                sampleProjects[group] = projects
    #print sampleProjects
    return sorted(sampleProjects.items(), key=lambda x: x[1]) # sort according to keys (folder name)

def getCoreSourceFiles(userIncludes = [], mcuPart="uc3l0128"):
    # scan all *.c files
    srcs = []
    required = glob.glob(BSP_DIR + '/*.c') + glob.glob(BSP_DIR + '/*.cpp')
    try:
        # print 'open SOURCES_' + mcuPart[:4].upper()
        f = open( os.path.join(ASF_DIR, 'SOURCES_' + mcuPart[:4].upper()), 'r' )
        for entry in f.readlines():
            src = os.path.join(ASF_DIR, entry.strip())
            if os.path.isfile(src) and not (src in required):
                required.append( src )
        f.close()
    except:
        pass

    for include in userIncludes:
        userheader = os.path.join( include[2:], os.path.split(include[2:])[1] + '.h' )
        try:
            fin = open(userheader, 'r')
            for line in fin.readlines():
                if line.replace(' ', '').find('#include') == 0: # found an '#include' directive
                    temp = line.strip()[len('#includes')-1 : ].strip()
                    header = temp[1:-1].strip() # get the header file
                    src = os.path.join(DRV_DIR, header[:-2], header[:-2]) + '.c' # e.g. drivers/adcifb/adcifb.c
                    print src
                    if os.path.isfile(src) and not (src in required):
                        required.append( src )
            fin.close()
        except:
            pass

    for fname in required:
        #srcs.append( os.path.join(os.getcwd(), fname) )
        srcs.append( fname )

    coresrcs = []
    for src in srcs:
        filepath = src.replace('\\', '/')
        if not (filepath in coresrcs):
            coresrcs.append(filepath)

    return coresrcs

def getIncludeDirs(mcuPart="uc3l0128"):
    dirs = [ BSP_DIR ]
    try:
        # print 'open INCLUDES_' + mcuPart[:4].upper()
        f = open( os.path.join(ASF_DIR, 'INCLUDES_' + mcuPart[:4].upper()), 'r' )
        for entry in f.readlines():
            path = os.path.join(ASF_DIR, entry.strip())
            if os.path.exists( path ):
                dirs.append( path )
        f.close()
    except:
        pass
    includes = []
    for d in dirs:
        includes.append( '-I' + d )
    #print includes
    return includes

# output: { pass, [includes], [sources] }
# [sources] contains the path name of the parsed used code
def parseUserCode(userCode=None, outPath=None, mcuPart="uc3l0128"):

    # check if user code (e.g. test.cxx) exists
    if not os.path.isfile(userCode): # file not found
        return False, [], []

    # create output directory, if not existing
    if not os.path.exists( outPath ):
        try:
            os.makedirs( outPath )
        except: # unable to create the directory
            return False, [], []

    # initial return values (empty)
    includes = []
    sources = [userCode]
    try:
        fin = open(userCode, 'r')
        for line in fin.readlines():
            if line.replace(' ', '').find('#include') == 0: # found an '#include' directive
                temp = line.strip()[len('#includes')-1 : ].strip()
                header = temp[1:-1].strip() # get the header file
                # print header
                libpath = USER_LIB_DIR + '/' + header[:-2]
                # check the folder and the header file if they exist
                if os.path.exists( libpath ) and os.path.isfile(libpath + '/' + header):
                    # print libpath
                    # todo: scan header file
                    include = '-I' + libpath
                    if not (include in includes): # include only once
                        includes.append( include )
                        for folder in ['/', '/Source/', '/src/', '/device/', '/host/']:
                            for ext in ['*.c', '*.cpp', '*.cxx']: # compile all *.c, *.cpp, *.cxx files
                                sources += glob.glob(libpath + folder + ext);
        fin.close()
    except:
        return False, [], []


    # lib core include paths and source files
    sources += getCoreSourceFiles(includes, mcuPart)
    includes += getIncludeDirs(mcuPart)

    return True, includes, sources

def getLinkerScript(mcuPart="uc3l0128"):
    #LINKER_SCRIPT = BSP_DIR + '/link_uc3l0128.lds'
    #return os.path.join( os.getcwd(), LINKER_SCRIPT )
    return BSP_DIR + '/link_' + str(mcuPart) + '.lds'

def getMcuArchitecture(mcuPart="uc3l0128"):
    if mcuPart[:4] == "uc3l":
        return "ucr3"
    if mcuPart[:4] == "uc3c":
        return "ucr3fp"
    return ""

def getCompilerDefines():
    defines = ''
    for flag, val in fwconfig.getDefines().items():
        defines += ' -D' + flag + '=' + val
    return defines


_clang_nodes=[]

def find_typerefs(node, fname=None):
    if str(node.location.file)==str(fname):
        knd = node.kind.name
        if knd!="PARM_DECL" and  knd!="INCLUSION_DIRECTIVE":
            #print node.kind, node.displayname, node.location
            if node.spelling:
                #print node.spelling
                _clang_nodes.append(str(node.spelling))
            else:
                #print node.displayname
                _clang_nodes.append(str(node.displayname))
    for c in node.get_children():
        find_typerefs(c, fname)

def getLibraryKeywords(headerFiles=[]):
    if not len(headerFiles):
        # search default header files
        headerFiles = glob.glob( BSP_DIR + '/*.h' )
        headerFiles += glob.glob( DRV_DIR + '/*/*.h' )

    #print headerFiles
    fwconfig.saveFwSettings()
    #print getCompilerDefines()

    del _clang_nodes[:]
    index = clang.Index.create()
    for fname in headerFiles:
        tu = index.parse(fname, [getCompilerDefines()], [], 0xFF)
        find_typerefs(tu.cursor, tu.spelling)

    keywords = list(set(_clang_nodes)) # remove duplicate items
    #print keywords
    #print 'found %d keywords' %len(keywords)
    return keywords

class FirmwareLibUpdate(QtCore.QThread):
    version_file = 'hardware/cores/bsp/version'
    version_url = 'http://yus-repo.googlecode.com/svn/trunk/AVR32/avr32-ide/' + version_file
    history_url = 'http://yus-repo.googlecode.com/svn-history/'

    corelib = '/trunk/AVR32/avr32-ide/hardware/cores/'
    userlib = '/trunk/AVR32/avr32-ide/libraries/'
    examples = '/trunk/AVR32/avr32-ide/examples/'

    href_str = '<li><a href="'
    file_list = []
    revision = 0
    revisionList = []

    def __init__(self, parent=None):
        QtCore.QThread.__init__(self, parent)
        self.parent = parent

        self.LogList = QtCore.QStringList()

    def setDesiredRevision(self, rev):
        self.revision = rev

    def getLog(self):
        if self.LogList.count()>0:
            return str(self.LogList.takeFirst())
        return None

    def run(self):
        self.LogList.clear()
        self.LogList.append('searching AVR32-GCC-IDE repository. please wait....')
        latest_rev = self.latest_fwlib_svnrev()
        # print 'latest_fwlib_svnrev = ', latest_rev
        if latest_rev == -1: # e.g. network error
            self.LogList.append('error: unable to reach repository!')
            return
        elif self.revision == 0:
            self.LogList.append('updating to latest(svn-%s). please wait...' % self.revisionList[latest_rev] )
            self.revision = latest_rev
        elif self.revision <= latest_rev:
            self.LogList.append('updating to svn-%s. please wait...' % self.revisionList[self.revision] )
        else: # exceeds latest
            self.LogList.append('abort update! latest is svn-%s '% self.revisionList[latest_rev] )
            return
        self.msleep(2000)

        dl_folder = 'tmp/fwlib_v%d_'%self.revision + str(QtCore.QDateTime.currentDateTime().toString('yyyyMMdd_hhmmss'))
        if not os.path.exists(dl_folder):
            try:
                os.makedirs(dl_folder)
            except:
                self.LogList.append('unable to create temporary download folder!')
                return

        done, total = 0, 0
        self.LogList.append('downloading new files. please wait...')
        updated, count = self.download_corelib(os.path.join(dl_folder,'hardware/cores'))
        done += updated
        total += count
        updated, count = self.download_userlib(os.path.join(dl_folder,'libraries'))
        done += updated
        total += count
        updated, count = self.download_examples(os.path.join(dl_folder,'examples'))
        done += updated
        total += count

        if total == 0:
            self.LogList.append('error: no files saved!')
            return
        elif done < total:
            self.LogList.append('error: failed to download all files!')
            return

        bkp_folder = 'tmp/fwlib_backup_' + str(QtCore.QDateTime.currentDateTime().toString('yyyyMMdd_hhmmss'))
        try:
            shutil.move('hardware/cores', os.path.join(bkp_folder,'hardware/cores'))
            shutil.move('libraries', os.path.join(bkp_folder,'libraries'))
            shutil.move('examples', os.path.join(bkp_folder,'examples'))
        except:
            self.LogList.append('warning: failed to move previous library files to backup folder!')

        try:
            shutil.copytree(os.path.join(dl_folder,'hardware/cores'), 'hardware/cores')
            shutil.copytree(os.path.join(dl_folder,'libraries'), 'libraries')
            shutil.copytree(os.path.join(dl_folder,'examples'), 'examples')
        except:
            self.LogList.append('error: failed to copy new library files!')
            return

        self.LogList.append('done updating to svn-%s. ( %d/%d files saved. )'%(self.revisionList[self.revision], done, total))
        self.msleep(2000)

    def latest_fwlib_svnrev(self):
        del self.revisionList[:]
        try:
            ver_lines = urllib2.urlopen(self.version_url).readlines()
            latest = -1
            for line in ver_lines:
                txt = line.strip()
                if not txt:
                    continue
                if latest < 0:
                    latest = int( txt.split(' ')[0] )
                else:
                    svnrev = txt.split(' ')[1]
                    self.revisionList.insert( int( txt.split(' ')[0] ), svnrev[svnrev.rfind('r'):] )
            # print self.revisionList
            return latest
        except:
            return -1

    def getCurrentRevision(self):
        try:
            f = open(self.version_file)
            txt = f.readline()
            f.close()
            rev = float(txt[:txt.find(' ')])
            return 'v%.2f'%(rev/100.0)
        except:
            return None

    def _browse(self, url):
        try:
            for line in urllib2.urlopen(url).readlines():
                href_pos = line.find(self.href_str)
                if href_pos>=0:
                    fname = line[href_pos+len(self.href_str):line.find('">')]
                    if not fname[0].isalnum():
                        continue
                    if fname.find('/')>0: # folder
                        self._browse(url+fname) # recursive
                    else:
                        self.file_list.append( url + fname )
        except:
            print 'unable to open: ', url

    def browse_corelib(self, rev=100):
        if rev > len(self.revisionList):
            return []
        url = self.history_url + '%s'%self.revisionList[rev] + self.corelib
        del self.file_list[:]
        self._browse(url)
        return self.file_list

    def browse_userlib(self, rev=100):
        if rev > len(self.revisionList):
            return []
        url = self.history_url + '%s'%self.revisionList[rev] + self.userlib
        del self.file_list[:]
        self._browse(url)
        return self.file_list

    def browse_examples(self, rev=100):
        if rev > len(self.revisionList):
            return []
        url = self.history_url + '%s'%self.revisionList[rev] + self.examples
        del self.file_list[:]
        self._browse(url)
        return self.file_list

    def _download(self, pre_url, folder):
        updated = 0
        for fname in self.file_list:
            lib_pos = fname.find(pre_url)
            if lib_pos>0:
                dst = os.path.join(folder, fname[lib_pos+len(pre_url):])
                dst_folder = os.path.dirname(dst)
                if not os.path.exists(dst_folder):
                    try:
                        os.makedirs(dst_folder)
                    except:
                        print 'unable to create folder: ', dst_folder
                try:
                    fout = open(dst, 'wb')
                    fout.write( urllib2.urlopen(fname).read() )
                    fout.close()
                    updated += 1
                    self.LogList.append('updated (v%d): %s' %(self.revision, os.path.basename(dst)))
                except:
                    print 'unable to save: ', dst
        return updated, len(self.file_list)

    def download_corelib(self, folder='hardware/cores'):
        self.browse_corelib(self.revision)
        return self._download(self.corelib, folder)

    def download_userlib(self, folder='libraries'):
        self.browse_userlib(self.revision)
        return self._download(self.userlib, folder)

    def download_examples(self, folder='examples'):
        self.browse_examples(self.revision)
        return self._download(self.examples, folder)

