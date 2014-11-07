
'''

    @filename: outline.py
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

import clang.cindex as clang
from firmware import getCompilerDefines
from PyQt4 import QtGui, QtCore

class CLangParserThread(QtCore.QThread):
    def __init__(self, parent=None):
        QtCore.QThread.__init__(self, parent)
        self.parent = parent
        self.queue = False
        self.nodes = []
        self.index = clang.Index.create()

    def find_typerefs(self, node, fname=None, level=0):
        if str(node.location.file)==str(fname):
            knd = node.kind.name
            if knd!="MACRO_INSTANTIATION" and \
                    knd!="TYPE_REF" and \
                    knd!="UNEXPOSED_EXPR" and \
                    knd!="DECL_REF_EXPR" and \
                    knd!="MEMBER_REF_EXPR" and \
                    knd!="CALL_EXPR" :
                self.nodes.append((node, level))
        for c in node.get_children():
            self.find_typerefs(c, fname, level+1)

    def run(self):
        self.msleep(200)
        del self.nodes[:]
        #print 'started parsing...'
        tu = self.index.parse('a.cpp',
                              getCompilerDefines().split(' '),
                              [('a.cpp', self.parent.getRawContent())],
                              13 )
        self.find_typerefs(tu.cursor, tu.spelling)
        #print 'done parsing.'
        self.msleep(200)

        if self.queue:
            self.queue = False
            self.run()
            return
        # print 'CLangParserThread done.'

    def setQueue(self):
        if self.queue:
            return
        self.queue = True;

    def getNodes(self):
        if self.isRunning():
            return []
        return self.nodes


class OutLineView(QtGui.QDockWidget):

    def __init__(self, parent=None):
        super(OutLineView, self).__init__("Outline Pending...", parent)
        self.parent = parent
        self.parser = CLangParserThread(self)
        self.itemStack = []
        self.updated = True

        self.treeWidget = QtGui.QTreeWidget(self)
        self.treeWidget.setHeaderHidden(True)
        self.treeWidget.itemClicked.connect(self.onItemDoubleClicked)
        self.setWidget(self.treeWidget)

        self.startTimer(2000)


    def update(self, newIndex=0):
        self.updated = False
        if self.parser.isRunning():
            self.parser.setQueue()
            return
        self.parser.start()

    def getRawContent(self):
        try:
            child = self.parent.currentWidget()
            if child:
                return str( child.text() )
        except:
            pass
        return 'empty'

    def onItemDoubleClicked(self, item=None, column=-1):
        if item:
            try:
                tip = str( item.toolTip(0) )
                child = self.parent.currentWidget()
                if child:
                    line = int( tip[tip.find('[')+1:tip.rfind(',')] ) - 1
                    col = int( tip[tip.rfind(' '):tip.rfind(']')] ) - 1
                    if tip.find('INCLUSION_DIRECTIVE')>0:
                        col += len('#include <')
                    child.setSelection( line, col, line, col+len(item.text(0)) )
            except:
                print "error: editor setSelection() failed!"

    def timerEvent(self, *args, **kwargs):
        if not self.updated:
            previous_level = 0
            nodes = self.parser.getNodes()
            self.setWindowTitle("Outline Pending...")
            if nodes:
                self.treeWidget.clear()
                for node, level in nodes:
                    if node.spelling:
                        label = str(node.spelling)
                    elif node.displayname:
                        label = str(node.displayname)
                    else:
                        continue

                    try:
                        tip = "%s : %s [%d, %d]"%(label, node.kind.name, node.location.line, node.location.column)
                        if node.kind.name=='VAR_DECL' and level>3:
                            level = level - 2
                        if level==1:
                            del self.itemStack[:]
                            item = QtGui.QTreeWidgetItem(self.treeWidget, [label])
                            item.setToolTip(0, tip)
                            #print 'add top...', label
                            self.treeWidget.addTopLevelItem(item)
                            self.itemStack.append(item)
                        elif level>previous_level:
                            while len(self.itemStack)<level-1:
                                self.itemStack.append(self.itemStack[len(self.itemStack)-1])
                            item = QtGui.QTreeWidgetItem([label])
                            item.setToolTip(0, tip)
                            #print 'add child...', self.itemStack[level-2].text(0)
                            self.itemStack[level-2].addChild( item )
                            self.itemStack.append( item )
                        elif level==previous_level:
                            item = QtGui.QTreeWidgetItem([label])
                            item.setToolTip(0, tip)
                            #print 'add sibling...', self.itemStack[level-2].text(0)
                            self.itemStack[level-2].addChild( item )
                            old_item = self.itemStack.pop()
                            del old_item
                            self.itemStack.append( item )
                        else: # level>previous_level:
                            while len(self.itemStack):
                                old_item = self.itemStack.pop()
                                del old_item
                                if len(self.itemStack)<level:
                                    item = QtGui.QTreeWidgetItem([label])
                                    item.setToolTip(0, tip)
                                    #print 'add to previous parent...', self.itemStack[level-2].text(0)
                                    self.itemStack[level-2].addChild( item )
                                    break;
                        # print level, label, node.kind, node.location.line
                    except:
                        print 'error adding %s, level(%d->%d), stack(%d)' %(label, previous_level, level, len(self.itemStack))
                    previous_level = level

                self.setWindowTitle("Outline")
                self.updated = True

            elif self.parser.isFinished():
                self.treeWidget.clear()
                self.setWindowTitle("Outline")
                self.updated = True

        #return QtGui.QDockWidget.timerEvent(self, *args, **kwargs)

