
'''

    @filename: cppeditor.py
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

import os
from PyQt4 import QtGui, QtCore
from PyQt4.Qsci import QsciScintilla, QsciLexerCPP, QsciAPIs
from firmware import getLibraryKeywords, REQUIRED_INCLUDE, USER_CODE_EXT

# user source code
PROJECT_ALIAS = 'AVR32 C++ Project' #'PhilRobokit Project'
PROJECT_NONAME = 'untitled'

__default_content__ = REQUIRED_INCLUDE + \
'''

int main(void)
{
\t// setup
\t
\twhile(true)
\t{
\t\t// loop
\t}
}
'''

class CppEditor(QsciScintilla):
    '''
    classdocs
    '''
    def __init__(self, parent=None, fileName=None, readOnlyFiles=[]):
        '''
        Constructor
        '''
        super(CppEditor, self).__init__(parent)
        self.parent = parent
        self.roFiles = readOnlyFiles

        self.setAcceptDrops(False) # drag&drop is on its parent

        # Set the default font
        font = QtGui.QFont()
        font.setFamily('Courier')
        font.setFixedPitch(True)
        font.setPointSize(10)
        self.setFont(font)
        self.setMarginsFont(font)

        # C/C++ lexer
        self.lexer = QsciLexerCPP(self,  True)
        self.lexer.setDefaultFont(font)
        self.libraryAPIs = QsciAPIs(QsciLexerCPP(self,True))
        self.setLexer(self.lexer)
        self.SendScintilla(QsciScintilla.SCI_STYLESETFONT, 1, 'Courier')

        # Auto-indent
        self.setTabWidth(4)
        #self.setIndentationsUseTabs(False)
        self.setAutoIndent(True)

        # Current line visible with special background color
        self.setCaretLineVisible(True)
        self.setCaretLineBackgroundColor(QtGui.QColor("#ffe4e4"))

        # Enable brace matching
        self.setBraceMatching(QsciScintilla.SloppyBraceMatch)

        # Enable folding visual- use boxes
        self.setFolding(QsciScintilla.BoxedTreeFoldStyle)

        # show line numbers
        fontmetrics = QtGui.QFontMetrics(font)
        self.setMarginsFont(font)
        self.setMarginWidth(0, fontmetrics.width("00000") + 4)
        self.setMarginLineNumbers(0, True)
        self.setMarginsBackgroundColor(QtGui.QColor("#ccccee"))

        # not too small
        self.setMinimumSize(400, 200)

        # set the length of the string before the editor tries to auto-complete
        self.setAutoCompletionThreshold(3)
        # tell the editor we are using a QsciAPI for the auto-completion
        self.setAutoCompletionSource(QsciScintilla.AcsAPIs)
        # removed remaining right side characters from the current cursor
        self.setAutoCompletionReplaceWord(True)

        # "CTRL+Space" autocomplete
        self.shortcut_ctrl_space = QtGui.QShortcut(QtGui.QKeySequence("Ctrl+Space"), self)
        self.connect(self.shortcut_ctrl_space,
            QtCore.SIGNAL('activated()'), self.autoCompleteFromAll)

        if fileName:
            self.curFile = fileName
            self.loadFile(fileName)
            self.isUntitled = False
        else:
            self.curFile = PROJECT_NONAME + USER_CODE_EXT
            self.setText( __default_content__ )
            self.isUntitled = True

        self.updateApiKeywords()
        self.isModified = False
        self.connect(self, QtCore.SIGNAL('textChanged()'), self.onTextChanged )

    def onTextChanged(self):
        self.isModified = True
        self.parent.onChildContentChanged()

    def loadFile(self, fileName):
        qfile = QtCore.QFile(fileName)
        if not qfile.open(QtCore.QFile.ReadOnly | QtCore.QFile.Text):
            QtGui.QMessageBox.warning(self, PROJECT_ALIAS,
                    "Cannot read file %s:\n%s." % (fileName, qfile.errorString()))
            return False

        ret = True
        try:
            # workaround for OS X QFile.readAll()
            f = open(qfile.fileName(), 'r')
            self.clear()
            for line in f.readlines():
                self.append(line)
        except:
            QtGui.QMessageBox.warning(self, PROJECT_ALIAS,
                    "failed to read %s." % fileName )
            ret = False
        finally:
            f.close()

        qfile.close()
        return ret

    def saveFile(self, fileName):
        if str(fileName).find(' ')>=0:
            QtGui.QMessageBox.warning(self, PROJECT_ALIAS,
                    'File path "%s" contains space(s). Please save to a valid location.'%fileName)
            return None
        qfile = QtCore.QFile(fileName)
        if not qfile.open(QtCore.QFile.WriteOnly | QtCore.QFile.Text):
            QtGui.QMessageBox.warning(self, PROJECT_ALIAS,
                    "Cannot write file %s:\n%s." % (fileName, qfile.errorString()))
            return None

        try:
            qfile.writeData(self.text())
        except:
            QtGui.QMessageBox.warning(self, PROJECT_ALIAS,
                    "Failed to save %s." % fileName )
            qfile.close()
            return None

        qfile.close()
        self.curFile = fileName
        self.isUntitled = False
        self.isModified = False
        return fileName

    def saveAs(self):
        fileName = QtGui.QFileDialog.getSaveFileName(self, "Save As",
                self.curFile, PROJECT_ALIAS + " (*" + USER_CODE_EXT + ");;" +
                    "C source (*.c);;C++ source (*.cpp);;Text File (*.txt);;All files (*.*)" )
        if not fileName:
            return None
        return self.saveFile(fileName)

    def save(self):
        f1 = os.path.abspath(self.curFile)
        for fname in self.roFiles:
            if f1 == os.path.abspath(fname): # same file
                if QtGui.QMessageBox.question(self.parent, "Project is read-only",
                        "This project is marked as \"read-only\".\n" + \
                        "Please click \"Cancel\" and save this to another location.\n\n" + \
                        "Continue saving the current project anyway?", "OK", "Cancel"):
                    return None
                #return self.saveAs()
                return self.saveFile(self.curFile)
        if self.isUntitled:
            return self.saveAs()
        else:
            return self.saveFile(self.curFile)

    def currentFile(self):
        return self.curFile

    def modified(self):
        return self.isModified

    def updateApiKeywords(self):
        self.libraryAPIs.clear()
        self.apiKeywords = self.parent.getDefaultKeywords()
        headerfiles = []
        for line in range(self.lines()):
            txt = str(self.text(line)).strip()
            if txt.find('int') == 0: # e.g. reached "int main()"
                break
            elif txt.find('#include') == 0:
                txt = ''.join(txt.split())
                temp = txt[len('#includes')-1 : ]
                header = temp[1:-1] # get the header file
                hfile = os.path.join('libraries', header[:-2], header)
                if os.path.isfile(hfile):
                    if not (hfile in headerfiles): headerfiles.append( hfile )
        if len( headerfiles ):
            #print 'parsing: ', headerfiles
            self.apiKeywords += getLibraryKeywords( headerfiles )
        #self.apiKeywords = list(set(self.apiKeywords)) # remove duplicates
        for keyword in self.apiKeywords:
            self.libraryAPIs.add( keyword )
        self.libraryAPIs.prepare()
        self.lexer.setAPIs(self.libraryAPIs)

    def insertIncludeDirective(self, library=''):
        directive =  '#include <' + library + '.h>\r\n'
        insert_pos = 0
        found_inc = False
        for line in range(self.lines()):
            txt = str(self.text(line)).strip()
            if txt.find('int') == 0: # e.g. reached "int main()"
                insert_pos = line - 1
                break
            elif txt.find('#include') == 0:
                found_inc = True
            elif found_inc:
                insert_pos = line
                break
        if insert_pos < 0 or insert_pos >= self.lines():
            insert_pos = 0
        self.insertAt(directive, insert_pos, 0)
        self.updateApiKeywords()

