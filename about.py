
'''

    @filename: about.py
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

from PyQt4 import QtGui, QtCore
from firmware import FirmwareLibUpdate

SPLASH_IMAGE = 'images/about.png'

SPLASH_NOTICE = '''
  AVR32 GCC IDE  Copyright (C) 2014  PhilRobotics
  This program comes with ABSOLUTELY NO WARRANTY.
  This is free software, and you are welcome
  to redistribute it under certain conditions.



'''

class AboutDialog(QtGui.QSplashScreen):
    ide_revision = "(unknown)"
    def __init__(self, parent=None):
        QtGui.QSplashScreen.__init__(self, parent, flags=QtCore.Qt.WindowStaysOnTopHint)

        self.pix = QtGui.QPixmap( SPLASH_IMAGE )
        self.setPixmap(self.pix)
        self.setMask( self.pix.mask() )

        self.showMessage( SPLASH_NOTICE + '    loading modules . . .',
            QtCore.Qt.AlignLeft | QtCore.Qt.AlignBottom, QtGui.QColor("#eecc77"))

        self.fw_update = FirmwareLibUpdate(self)
        self.input_dlg = QtGui.QInputDialog(self)
        self.fw_update_timer_id = None

    def getVersions(self):
        fw_rev = self.fw_update.getCurrentRevision()
        return 'build: %s   Library: %s' %(self.ide_revision, fw_rev)

    def mousePressEvent(self, *args, **kwargs):
        # print 'you pressed me!'
        if not self.fw_update.isRunning():
            self.close()
            return QtGui.QSplashScreen.mousePressEvent(self, *args, **kwargs)

    def setMsg(self, msg):
        self.showMessage( '[developer mode] '+ msg, QtCore.Qt.AlignLeft | QtCore.Qt.AlignBottom, QtGui.QColor("#eecc77"))


    def showUpdateDialog(self):
        rev, res = self.input_dlg.getInteger(self, 'Update FW Lib', 'Input version ("0" = get latest)', 0, 0, 10000)
        if res:
            self.fw_update.setDesiredRevision(rev)
            self.fw_update.start()
            self.fw_update_timer_id = self.startTimer(200)

    def timerEvent(self, *args, **kwargs):
        timerID = args[0].timerId()
        if timerID == self.fw_update_timer_id:
            msg = self.fw_update.getLog();
            if msg:
                self.setMsg(msg)
            elif not self.fw_update.isRunning():
                self.killTimer(self.fw_update_timer_id)
        #return QtGui.QSplashScreen.timerEvent(self, *args, **kwargs)


