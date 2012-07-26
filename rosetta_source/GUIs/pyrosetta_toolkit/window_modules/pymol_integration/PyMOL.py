#!/usr/bin/python
# (c) Copyright Rosetta Commons Member Institutions.
# (c) This file is part of the Rosetta software suite and is made available under license.
# (c) The Rosetta software is developed by the contributing members of the Rosetta Commons.
# (c) For more information, see http://www.rosettacommons.org. Questions about this can be
# (c) addressed to University of Washington UW TechTransfer, email: license@u.washington.edu.

## @file   /GUIs/pyrosetta_toolkit/window_modules/pymol_integration/PyMOL.py
## @brief  Main window and controller of PyMol integration accross the GUI
## @author Jared Adolf-Bryfogle (jadolfbr@gmail.com)

from rosetta import *
from Tkinter import *
import glob
import tkFileDialog
import tkMessageBox
import tkSimpleDialog
import os



class AdvancedPyMOL():
    '''
    Advance Pymol Visualization Window
    Version 2.0 ONLY
    '''

    def __init__(self, pose):
	'''
	This object should handle all pymol stuff across the board eventually.
	'''

	self.auto_send = IntVar()
	self.auto_send.set(0)
	self.pymover = PyMOL_Mover()
	self.pymover.keep_history(True)
	self.observer = PyMOL_Observer()
	self.observer.pymol.update_energy=True
	#Trace this - Add observer when the value changes, or remove the observer when nessessary.
	self.auto_send.trace_variable('w', self.auto_change_observer)
	self.pnum = 0
	self.pose = pose
	
	self.pymol_functions = {
	"View Residue Energy": lambda:self.pymover.send_energy(self.pose),
	"Send Hydrogen Bonds": lambda:self.pymover.send_hbonds(self.pose),
	"Send Polar Identity": lambda:self.pymover.send_polars(self.pose),
	"View Foldtree":lambda: self.pymover.view_foldtree(self.pose)
	}
	
    def auto_change_observer(self, name, index, mode):
	varValue = self.auto_send.get()
	if not self.auto_send.get():
	    self.observer.remove_observer(self.pose)
	else:
	    print "Added observer"
	    self.observer.add_observer(self.pose)
    def makeWindow(self, row, column, main, ScoreObject):

	self.main = main
	self.main.title("Advanced PyMOL Visualization")
	try :
	    print self.pose.pdb_info().name()
	except AttributeError:
	    tkMessageBox.showwarning(message = 'Please Load a Pose...')
	    return
	self.ScoreObject = ScoreObject; self.score = self.ScoreObject.score

	self.viewlabel_ = Label(self.main, text="View Options")
	self.scorelabel_ = Label(self.main, text="Score Terms")
	self.viewList = Listbox(self.main)
	self.scoreList = Listbox(self.main)
	self.sendbutton_ = Button(self.main, text = "Send Pose", command = lambda: self.SendPose(self.viewList.get(self.viewList.curselection())))
	self.sendnewbutton_ = Button(self.main, text = "Send New Pose", command = lambda: self.SendNewPose())
	self.autocheck_button_ck = Checkbutton(self.main, text = "Auto Send Pose as new Object", variable = self.auto_send)
	self.viewlabel_.grid(row = row+1, column=column); self.scorelabel_.grid(row = row+1, column = column+2)
	self.viewList.grid(row = row+2, column=column); self.scoreList.grid(row = row+2, column = column+2)
	self.sendbutton_.grid(row = row+4, column = column+1); self.sendnewbutton_.grid(row = row+3, column = column+1); self.autocheck_button_ck.grid(row = row, column=column+2)
	for option in self.pymol_functions:
	    self.viewList.insert(END, option)

	ZeroTerms, NonZeroTerms = ScoreObject.scoreOption("Breakdown ScoreFxn")
	for option in NonZeroTerms:
	    self.scoreList.insert(END, option)

	self.viewList.bind("<Double-Button-1>", lambda event: self.SendPose(self.viewList.get(self.viewList.curselection())))
	self.scoreList.bind("<Double-Button-1>", lambda event: self.SendEPose(self.scoreList.get(self.scoreList.curselection())))
    def SendPose(self, option):
	'''
	Sends pose according to option. Will Fix as callback.
	'''
	print self.score(self.pose)
	
	func = self.pymol_functions[option]
	try:
	    func()
	    return
	except AttributeError:
	    print "Function not working in this version of PyRosetta...."
	    return
	
    def SendEPose(self, option):
	optionSP = option.split(";")
	term = optionSP[0].lstrip()
	print term
	#vars()[term]
	print self.score(self.pose)
	self.pymover.send_energy(self.pose, term)
    def SendNewPose(self):
	'''
	Changes the name of the pose so that the next thing sent is a new object...
	'''
	self.pnum = self.pnum+1
	Newname = os.path.basename(self.pose.pdb_info().name()).split("_")[0]+"_"+repr(self.pnum)
	self.pose.pdb_info().name(Newname)
	self.pymover.apply(self.pose)
	return
