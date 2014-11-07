
'''

    @filename: mainwindow.py
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

import os, functools
from PyQt4 import QtGui, QtCore
from editor import MultipleCppEditor
from firmware import scanFirmwareLibs, getExampleProjects
from compiler import GccCompilerThread
from configs import IdeConfig
from serialport import scan_serialports, SerialPortMonitor
from about import AboutDialog, SPLASH_NOTICE


class AppMainWindow(QtGui.QMainWindow):
    '''
    classdocs
    '''

    def __init__(self):
        '''
        Constructor
        '''
        super(AppMainWindow, self).__init__()
        print "AVR32 GCC IDE started..."
        print SPLASH_NOTICE

        if False: # todo: set to True if building stand-alone package (cx_Freeze)
            setpath = os.path.dirname( os.path.realpath( __file__ ) )
            if os.name == 'nt':
                os.chdir( setpath[:setpath.rfind('\\')] )
            else:
                os.chdir( setpath[:setpath.rfind('/')] )

        self.aboutDlg = AboutDialog(self)
        self.aboutDlg.show()

        self.setWindowTitle("AVR32 GCC IDE")
        self.setWindowIcon(QtGui.QIcon('images/app.png'))
        self.setMinimumSize(300, 400)

        self.Editor = MultipleCppEditor(self)
        self.setCentralWidget(self.Editor)

        self.OutLineView =  self.Editor.getOutLineView()
        self.OutLineView.setObjectName("OutLineView")
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, self.OutLineView)

        self.Compiler = GccCompilerThread(self)
        self.McuPartName = None
        self.pollCompilerTimerID = None

        self.serialPortName = None
        self.serialPortLabel = QtGui.QLabel('<font color=red><i>(select port)</i></font>')
        self.SerialPortMonitorDialog = SerialPortMonitor(self)

        self.Configs = IdeConfig(self)

        self.createLogWindow()
        self.createActions()
        self.createToolBars()
        self.createStatusBar()

        self.Configs.restoreIdeSettings()
        self.createMenus()

        self.aboutDlg.finish(self)
        print "IDE ready."


    def about(self):
        self.aboutDlg.show()
        kdbMod = QtGui.QApplication.keyboardModifiers()
        if kdbMod == QtCore.Qt.ShiftModifier:
            self.aboutDlg.showMessage('[developer mode] update firmware library...', QtCore.Qt.AlignLeft | QtCore.Qt.AlignBottom, QtGui.QColor("#eecc77"))
            self.aboutDlg.showUpdateDialog()
            return
        # todo: other informations
        self.aboutDlg.showMessage("AVR32 GCC IDE [ %s ]" % self.aboutDlg.getVersions(),
                           QtCore.Qt.AlignLeft | QtCore.Qt.AlignBottom, QtGui.QColor("#eecc77"))

    def openProjectProtoSite(self):
        QtGui.QDesktopServices.openUrl( QtCore.QUrl("http://projectproto.blogspot.com") )

    def openPhilRoboticsSite(self):
        # todo: change to .ORG
        QtGui.QDesktopServices.openUrl( QtCore.QUrl("http://www.philrobotics.com") )
    def aboutCompiler(self):
        info = self.Compiler.getCompilerInfo()
        #self.log.append(info)
        if info:
            QtGui.QMessageBox.about( self, "Compiler Information", info )
        else:
            QtGui.QMessageBox.about( self, "Compiler Information", "no compiler found!" )

    def startBuild(self):
        if self.Compiler.isRunning():
            self.insertLog('compiler busy..')
            return
        if not self.McuPartName or not self.boardGroup.checkedAction():
            self.insertLog('<font color=orange>Please select first a Target Board.</font>')
            return
        kdbMod = QtGui.QApplication.keyboardModifiers()
        if not os.path.isfile(self.Editor.getCurrentFile()) or self.Editor.isCurrentFileModified():
            ret = self.Editor.saveFile() # save the file first before starting the build.
            if ret == False:
                self.insertLog("<font color=red>unable to save project!</font>")
                return
            elif ret == None:
                self.insertLog("nothing to build.")
                return
        self.insertLog("<font color=green>------- Start Project Build. -------</font>", True)
        fn = self.Editor.getCurrentFile()
        cleanBuild = False
        if kdbMod == QtCore.Qt.ShiftModifier:
            #print 'Shift-Click PushButton'
            cleanBuild = True
        ret, msg = self.Compiler.buildProject(self.McuPartName, fn, cleanBuild)
        if not ret:
            self.insertLog( "<font color=red>%s</font>"%msg )
            if msg == "file not found":
                QtGui.QMessageBox.warning( self, "Build Error", "File not found (may be unsaved yet). " + \
                                             "Create or save first the file." )
            elif msg == "busy":
                QtGui.QMessageBox.warning( self, "Busy", "Previous build process still running!" )
            elif msg == "abort":
                QtGui.QMessageBox.warning( self, "Error", "Unable to start build process!" )
        else:
            self.insertLog( "<font color=lightblue><i>   %s   </i></font>"%msg )
            self.pollCompilerTimerID = self.startTimer(50)
            if not self.pollCompilerTimerID:
                self.insertLog("<font color=red>Unable to start Timer.</font>")

    def stopBuild(self):
        if self.pollCompilerTimerID:
            self.killTimer(self.pollCompilerTimerID)
            self.pollCompilerTimerID = None
            self.Compiler.pollBuildProcess(True)
            self.insertLog("<font color=red>----- Stopped. -----</font>")
        else:
            self.insertLog("nothing to stop.")

    def programChip(self):
        if self.Editor.isCurrentFileModified():
            self.insertLog('<font color=orange>Project was modified. Please re-build the project.</font>')
            return
        if not self.McuPartName or not self.boardGroup.checkedAction():
            self.insertLog('<font color=orange>Please select first a Target Board.</font>')
            return
        if not self.serialPortName:
            self.insertLog('<font color=orange>Please select first a Serial Port.</font>')
            return
        if self.Compiler.isRunning():
            self.insertLog('compiler/bootloader busy... please wait...')
            return
        if self.SerialPortMonitorDialog.isPortOpen():
            self.SerialPortMonitorDialog.close() # close first serial port monitor
        fn = self.Editor.getCurrentFile()
        ret, msg = self.Compiler.programHex( self.McuPartName, fn, self.serialPortName )
        if ret:
            self.insertLog("<font color=green>Bootload/Program Device:</font>", True)
            self.insertLog("<font color=lightblue><i>   %s   </i></font>"%msg)
            self.pollCompilerTimerID = self.startTimer(10)
            if not self.pollCompilerTimerID:
                self.insertLog("<font color=red>Unable to start Timer.</font>")
        else:
            self.insertLog("<font color=red>%s</font>"%msg)

    def selectMcuPart(self):
        act = self.boardGroup.checkedAction()
        if act:
            mcuname = str( act.text() ).split(' ')[1].lower()
            if mcuname != self.McuPartName:
                self.McuPartName = mcuname
                self.Configs.saveIdeSettings( mcuPartName=self.McuPartName )
                self.insertLog( 'selected mcu part: <b><font color=green>%s</font></b>' % self.McuPartName.upper() )

    def selectSerialPort(self):
        act = self.serialPortGroup.checkedAction()
        if act:
            portname = str( act.text() )
            if portname != self.serialPortName:
                self.serialPortName = portname
                self.Configs.saveIdeSettings( serialPortName=self.serialPortName )
                self.insertLog( 'selected port: <b><font color=green>%s</font></b>' % self.serialPortName )
                self.serialPortLabel.setText('<font color=green>%s</font>'%self.serialPortName)
                if self.SerialPortMonitorDialog.isPortOpen():
                    if not self.SerialPortMonitorDialog.openPort(self.serialPortName):
                        self.SerialPortMonitorDialog.close()
                        self.insertLog( "<font color=red>unable to open %s</font>"%self.serialPortName)

    def updateSerialPortList(self):
        # clear previous actions list
        self.serialPortMenu.clear()
        for act in self.serialPortGroup.actions():
            self.serialPortGroup.removeAction(act)
            del act

        # scan existing ports
        portList = scan_serialports() # serialport.py
        previousPortName = self.Configs.getSerialPortName()

        # create new actions & update serial port menu
        if len(portList):
            for i in range(len(portList)):
                act = QtGui.QAction(portList[i],  self, checkable=True,
                            statusTip="select " + portList[i] + " serial port",
                            triggered=self.selectSerialPort)
                self.serialPortGroup.addAction( act )
                self.serialPortMenu.addAction( act )
                if portList[i] == previousPortName:
                    act.setChecked(True)
                    act.trigger()

        if not self.serialPortGroup.checkedAction():
            self.serialPortName = ''
            self.insertLog( '<i><font color=gray>( please select a serial port. )</font></i>' )

    def importFirmwareLib(self, action=None):
        if action:
            libname = str( action.text() )
            self.Editor.importFirmwareLib(libname)

    def openSerialPortMonitorDialog(self):
        if self.serialPortName == None:
            self.insertLog( "<font color=red>no serial port selected!</font>" )
            return
        if self.SerialPortMonitorDialog.openPort(self.serialPortName):
            self.SerialPortMonitorDialog.show() # non-modal open
        else:
            self.insertLog( "<font color=red>unable to open serial port!</font>" )

    def createActions(self):
        # file menu
        self.newAct = QtGui.QAction( QtGui.QIcon("./images/new.png"), "&New",
                self, shortcut=QtGui.QKeySequence("Ctrl+N"),
                statusTip="Create a new file", triggered=self.Editor.newFile)
        self.openAct = QtGui.QAction(QtGui.QIcon("./images/open.png"), "&Open...",
                self, shortcut=QtGui.QKeySequence("Ctrl+O"),
                statusTip="Open an existing file")
        self.openAct.triggered.connect( functools.partial(self.Editor.openFile, None) )
        self.closeAct = QtGui.QAction("&Close",
                self, shortcut=QtGui.QKeySequence("Ctrl+W"),
                statusTip="Close the current window", triggered=self.Editor.closeCurrentFile)
        self.saveAct = QtGui.QAction(QtGui.QIcon("./images/save.png"), "&Save",
                self, shortcut=QtGui.QKeySequence("Ctrl+S"),
                statusTip="Save the current file", triggered=self.Editor.saveFile)
        self.saveAsAct = QtGui.QAction("Save &As...", self, shortcut=QtGui.QKeySequence("Ctrl+Shift+S"),
                statusTip="Save to another file", triggered=self.Editor.saveFileAs)

        self.exitAct = QtGui.QAction("E&xit", self,
                shortcut=QtGui.QKeySequence("Alt+F4"),
                statusTip="Exit the application", triggered=QtGui.qApp.closeAllWindows)

        # edit menu
        self.editUndoAct = QtGui.QAction("&Undo", self, shortcut=QtGui.QKeySequence("Ctrl+Z"),
                                         triggered=self.Editor.editUndo)
        self.editRedoAct = QtGui.QAction("&Redo", self, shortcut=QtGui.QKeySequence("Ctrl+Y"),
                                         triggered=self.Editor.editRedo)
        self.editCutAct = QtGui.QAction("Cu&t", self, shortcut=QtGui.QKeySequence("Ctrl+X"),
                                         triggered=self.Editor.editCut)
        self.editCopyAct = QtGui.QAction("&Copy", self, shortcut=QtGui.QKeySequence("Ctrl+C"),
                                         triggered=self.Editor.editCopy)
        self.editPasteAct = QtGui.QAction("&Paste", self, shortcut=QtGui.QKeySequence("Ctrl+V"),
                                         triggered=self.Editor.editPaste)
        self.editSelectAllAct = QtGui.QAction("Select &All", self, shortcut=QtGui.QKeySequence("Ctrl+A"),
                                         triggered=self.Editor.editSelectAll)
        self.editClearAct = QtGui.QAction("Clear", self,  triggered=self.Editor.editClear)
        # find/replace
        self.findAct = QtGui.QAction("&Find/Replace...", self,
                shortcut=QtGui.QKeySequence("Ctrl+F"),
                statusTip="Find/Replace texts", triggered=self.Editor.showFindDialog)

        # project menu
        self.compileAct = QtGui.QAction(QtGui.QIcon("./images/build.png"), "&Compile",
                self, shortcut=QtGui.QKeySequence("Ctrl+B"),
                statusTip="Build the current project", triggered=self.startBuild)
        self.stopAct = QtGui.QAction(QtGui.QIcon("./images/stop.png"), "S&top",
                self, statusTip="Cancel the build process", triggered=self.stopBuild)
        self.programAct = QtGui.QAction(QtGui.QIcon("./images/load.png"), "&Load",
                self, shortcut=QtGui.QKeySequence("Ctrl+R"),
                statusTip="Download program to the board using bootloader", triggered=self.programChip)

        self.firmwareLibList = scanFirmwareLibs()
        self.firmwareLibActs = []
        if len(self.firmwareLibList):
            for i in range(len(self.firmwareLibList)):
                self.firmwareLibActs.append(
                        QtGui.QAction(self.firmwareLibList[i],  self,
                            statusTip="include " + self.firmwareLibList[i] + " library" ) )

        self.exampleProjects = getExampleProjects(self.firmwareLibList)
        self.exampleFolderMenus = []
        self.openExampleActs = []
        for group in self.exampleProjects:
            folder, files = group[0], group[1]
            self.exampleFolderMenus.append(QtGui.QMenu(str(folder), self))
            for fname in files:
                baseName = os.path.basename(fname)
                self.openExampleActs.append(QtGui.QAction(os.path.splitext(baseName)[0], self,
                                statusTip = 'Open "' + str(fname).replace('\\', '/') + '"') )

        # serial monitor/terminal window
        self.serialMonitorAct = QtGui.QAction(QtGui.QIcon("./images/serial.png"), "Serial &Monitor",
                self, shortcut=QtGui.QKeySequence("Ctrl+Shift+M"),
                statusTip="Launch Serial Monitor Dialog", triggered=self.openSerialPortMonitorDialog)
        self.serialPortGroup = QtGui.QActionGroup(self)
        self.serialPortList = scan_serialports()
        self.serialPortActs = []
        if len(self.serialPortList):
            for i in range(len(self.serialPortList)):
                self.serialPortActs.append(
                        QtGui.QAction(self.serialPortList[i],  self, checkable=True,
                            statusTip="select " + self.serialPortList[i] + " serial port",
                            triggered=self.selectSerialPort) )
                self.serialPortGroup.addAction( self.serialPortActs[i] )

        # todo: board names??
        #self.boardAnitoAct = QtGui.QAction("PhilRobokit &Anito",  self,
        #        checkable=True, statusTip="Select PhilRobokit Anito board" )
        self.boardEgizmoUC3L0128Act = QtGui.QAction("eGizmo UC3L0128",  self, checkable=True,
                statusTip="Select eGizmo AT32UC3L0128 MCU board", triggered=self.selectMcuPart)
        self.boardEgizmoUCL0256Act = QtGui.QAction("eGizmo UC3L0256",  self, checkable=True,
                statusTip="Select eGizmo AT32UC3L0256 MCU board", triggered=self.selectMcuPart)
        self.boardEgizmoUC3C264Act = QtGui.QAction("eGizmo UC3C264C",  self, checkable=True,
                statusTip="Select eGizmo AT32UC3C264 MCU board", triggered=self.selectMcuPart)
        self.boardEgizmoUC3C2128Act = QtGui.QAction("eGizmo UC3C2128C",  self, checkable=True,
                statusTip="Select eGizmo AT32UC3C2128 MCU board", triggered=self.selectMcuPart)
        self.boardGroup = QtGui.QActionGroup(self)
        #self.boardGroup.addAction(self.boardAnitoAct)
        self.boardGroup.addAction(self.boardEgizmoUC3L0128Act)
        self.boardGroup.addAction(self.boardEgizmoUCL0256Act)
        self.boardGroup.addAction(self.boardEgizmoUC3C264Act)
        self.boardGroup.addAction(self.boardEgizmoUC3C2128Act)

        self.restoreDefaultsAct = QtGui.QAction("Restore Defaults",  self,
                statusTip="Clear configuration files", triggered=self.Configs.setDefaults)

        # help menu
        self.aboutAct = QtGui.QAction("&About", self, shortcut=QtGui.QKeySequence("F1"),
                statusTip="About the IDE", triggered=self.about)
        self.aboutCompilerAct = QtGui.QAction("About &Compiler", self,
                statusTip="About GNU tools for AVR32", triggered=self.aboutCompiler)
        self.aboutQtAct = QtGui.QAction("About &Qt", self,
                statusTip="Show the Qt library's About box", triggered=QtGui.qApp.aboutQt)
        self.visitProjectprotoSiteAct = QtGui.QAction("Visit &ProjectProto", self,
                statusTip="Open ProjectProto blog site (yus' projects)", triggered=self.openProjectProtoSite)
        self.visitPhilroboticsSiteAct = QtGui.QAction("Visit Phil&Robotics", self,
                statusTip="Open PhilRobotics website", triggered=self.openPhilRoboticsSite)

    def createMenus(self):
        ### File Menu ###
        self.fileMenu = self.menuBar().addMenu("&File")
        self.fileMenu.addAction(self.newAct)
        self.fileMenu.addAction(self.openAct)
        self.fileMenu.addSeparator()

        self.examplesMenu = QtGui.QMenu('Examples', self)
        fileCount = 0
        for dirCount in range( len(self.exampleFolderMenus) ):
            examples = self.exampleProjects[dirCount][1]
            for fname in examples:
                pathname =  str( os.getcwd() + '/' + fname ) # complete path
                pathname = pathname.replace('\\', '/') # for consistency
                self.openExampleActs[fileCount].triggered.connect(
                                        functools.partial(self.Editor.openFile, pathname) )
                self.exampleFolderMenus[dirCount].addAction(self.openExampleActs[fileCount])
                fileCount += 1
            self.examplesMenu.addMenu(self.exampleFolderMenus[dirCount])
        self.fileMenu.addMenu(self.examplesMenu)
        self.fileMenu.addSeparator()

        self.fileMenu.addAction(self.saveAct)
        self.fileMenu.addAction(self.saveAsAct)
        self.fileMenu.addAction(self.closeAct)
        self.fileMenu.addSeparator()
        self.fileMenu.addAction(self.exitAct)

        ### Edit Menu ###
        self.editMenu = self.menuBar().addMenu("&Edit")
        self.editMenu.addAction(self.editUndoAct)
        self.editMenu.addAction(self.editRedoAct)
        self.editMenu.addSeparator()
        self.editMenu.addAction(self.editCutAct)
        self.editMenu.addAction(self.editCopyAct)
        self.editMenu.addAction(self.editPasteAct)
        self.editMenu.addAction(self.editSelectAllAct)
        self.editMenu.addAction(self.editClearAct)
        self.editMenu.addSeparator()
        self.editMenu.addAction(self.findAct)

        ### Project Menu ###
        self.projectMenu = self.menuBar().addMenu("&Project")
        self.projectMenu.addAction(self.compileAct)
        self.projectMenu.addAction(self.stopAct)
        self.programMenu = self.projectMenu.addMenu("Program Board...")
        self.programMenu.addAction(self.programAct)
        self.projectMenu.addSeparator()
        self.firmwareLibMenu = self.projectMenu.addMenu("Import &Library...")
        if len(self.firmwareLibActs):
            for i in range(len(self.firmwareLibActs)):
                self.firmwareLibMenu.addAction(self.firmwareLibActs[i])
        self.connect(self.firmwareLibMenu,
                     QtCore.SIGNAL("triggered (QAction *)"), self.importFirmwareLib)

        ### Tools Menu ###
        self.toolsMenu = self.menuBar().addMenu("&Tools")
        self.toolsMenu.addAction(self.serialMonitorAct)
        self.toolsMenu.addSeparator()
        self.boardMenu = self.toolsMenu.addMenu("&Board")
        #self.boardMenu.addAction(self.boardAnitoAct)
        self.boardMenu.addAction(self.boardEgizmoUC3L0128Act)
        self.boardMenu.addAction(self.boardEgizmoUCL0256Act)
        self.boardMenu.addAction(self.boardEgizmoUC3C264Act)
        self.boardMenu.addAction(self.boardEgizmoUC3C2128Act)
        previousMcuPart = self.Configs.getMcuPartName()
        for act in self.boardGroup.actions():
            mcuname = str(act.text()).split(' ')[1].lower()
            if mcuname == previousMcuPart:
                act.setChecked(True)
                act.trigger()
                break

        self.serialPortMenu = self.toolsMenu.addMenu("&Serial Port")
        self.serialPortGroup = QtGui.QActionGroup(self)
        self.connect(self.serialPortMenu, QtCore.SIGNAL("aboutToShow ()"), self.updateSerialPortList )
        self.updateSerialPortList()
        self.toolsMenu.addSeparator()
        self.toolsMenu.addAction(self.restoreDefaultsAct) # todo: create settings dialog
        #self.bootloaderMenu = self.toolsMenu.addMenu("&Booloader")

        ### Help Menu ###
        self.helpMenu = self.menuBar().addMenu("&Help")
        self.helpMenu.addAction(self.visitPhilroboticsSiteAct)
        self.helpMenu.addAction(self.visitProjectprotoSiteAct)
        self.helpMenu.addAction(self.aboutCompilerAct)
        self.helpMenu.addAction(self.aboutQtAct)
        self.helpMenu.addAction(self.aboutAct)

    def createToolBars(self):
        self.fileToolBar = self.addToolBar("File")
        self.fileToolBar.setObjectName("FileToolBar")
        self.fileToolBar.addAction(self.newAct)
        self.fileToolBar.addAction(self.openAct)
        self.fileToolBar.addAction(self.saveAct)

        self.projectToolBar = self.addToolBar("Project")
        self.projectToolBar.setObjectName("ProjectToolBar")
        self.projectToolBar.addAction(self.compileAct)
        self.projectToolBar.addAction(self.stopAct)
        self.projectToolBar.addAction(self.programAct)

        self.serialToolBar = self.addToolBar("Serial Port")
        self.serialToolBar.setObjectName("SerialPortToolBar")
        self.serialToolBar.addAction(self.serialMonitorAct)
        self.serialToolBar.addWidget(self.serialPortLabel)

    def createStatusBar(self):
        self.statusBar().showMessage("Ready")

    def createLogWindow(self):
        self.log = QtGui.QTextEdit(self)
        self.log.setReadOnly(True)
        self.log.resize(self.width(), 100 )
        self.log.setText("Ready")
        palette = QtGui.QPalette(QtGui.QColor(0, 0, 0))
        palette.setColor(QtGui.QPalette.Active, QtGui.QPalette.Base, QtGui.QColor(25, 10, 0))
        self.log.setPalette(palette)
        logWindow = QtGui.QDockWidget("Log", self)
        logWindow.setObjectName("LogView")
        logWindow.setWidget(self.log)
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, logWindow)

    def insertLog(self, log='', resetWindow=False):
        if resetWindow:
            self.log.setText('')
        self.log.append(log)

    def timerEvent(self, *args, **kwargs):
        timerID = args[0].timerId()
        if timerID == self.pollCompilerTimerID:
            ret, msg = self.Compiler.pollBuildProcess()
            if ret:
                if len( msg ):
                    self.insertLog( msg )
            else:
                self.killTimer(timerID)
                self.pollCompilerTimerID = None

        #return QtGui.QMainWindow.timerEvent(self, *args, **kwargs)


    def closeEvent(self, event):
        if not self.Editor.closeAllTabs(): # check for unsaved changes in the project(s)
            event.ignore()
            return
        self.Configs.saveIdeSettings(self.serialPortName, self.McuPartName)
        return QtGui.QMainWindow.closeEvent(self, event)

