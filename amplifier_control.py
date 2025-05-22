#!/home/hododaq/anaconda3/bin/python
 
from __future__ import print_function

import sys
from PyQt5 import QtGui, QtCore, QtWidgets
import serial
import serial.tools.list_ports
import time
import os
import csv
import subprocess
import platform

#import ctypes

# Gain values are inverted for some reason (odd and even channels pairs are switched): chswap fixes this
chswap = []
for i in range(100):
    chswap.append(2 * i + 1)
    chswap.append(2 * i)

version = sys.version_info[:2]

getBoardID = bytearray(b'I\n')
getChannels = bytearray(b'G\n')
ledToggle = bytearray(b'L\n')
readValues = bytearray(b'R\n')
saveToEeprom = bytearray(b'SV\n')
writeValues = bytearray(b'W\n')
ACK = bytearray(b'\x06')
EOT = bytearray(b'\x04')

glist = []
tlist = []

def main():
    main.app = QtWidgets.QApplication(sys.argv)
    w = Window()
    w.setWindowTitle('Arduino Amplifier Control')
    w.setWindowIcon(QtGui.QIcon('icons/amp_icon.png'))
    w.show()
    sys.exit(main.app.exec_())


def findArduinos():
    """Returns a list of all connected Arduinos, filtered by VID:PID and sorted by their board IDs, in an array. First column contains board IDs, second column contains their COM ports."""
    arduinos = []
    boards = [comport.device for comport in serial.tools.list_ports.comports()]
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if"VID:PID=2341:8036" in str(port.hwid):        # ID of Arduino Leonardo
        # if "VID:PID=1A86:7523" in str(port.hwid):       # ID of Wemos D1 mini
            description = port.description
            pid = str(port.pid)
            vid = str(port.vid)
            device = port.device
            ser = serial.Serial(device, 9600, timeout=0.1)
            ser.write(getBoardID)
            val = ser.read()
            arduinos.append([ord(val), device])
    arduinos.sort()
    return arduinos


def writeSerial(port, value):
    ser = serial.Serial(str(port), 9600, timeout=0.1)
    # ser.write(ledToggle)


def toggleLED(serial):
    serial.write(ledToggle)


class Window(QtWidgets.QMainWindow):
    def __init__(self):
        QtWidgets.QMainWindow.__init__(self)
        Window.boardIDs = findArduinos()
        sipm()
        numArduinos = len(self.boardIDs)

        vlayout = QtWidgets.QVBoxLayout()
        mainInfo = QtWidgets.QLabel("Insert informational text<br><br><br><br>")


        # Top buttons
        self.hbox = QtWidgets.QHBoxLayout()
        vlayout.addLayout(self.hbox)

        self.refall = QtWidgets.QPushButton()
        self.refall.setIcon(QtGui.QIcon("icons/reficon.png"))
        self.refall.setToolTip("Load values from all boards to computer")
        self.refall.clicked.connect(
            lambda state: self.readValues())

        self.impall = QtWidgets.QPushButton() # Import to all boards
        self.impall.setIcon(QtGui.QIcon("icons/impicon.png"))
        self.impall.setToolTip("Import values from CSV file to set to all boards")
        self.impall.clicked.connect(
            lambda state: self.importValuesAll(sipm.numChannels[0][1])) # Imports based on # of channels of first board

        self.setandapplyall = QtWidgets.QPushButton()
        self.setandapplyall.setIcon(QtGui.QIcon("icons/checkicon.png"))
        self.setandapplyall.setToolTip("Send gain and threshold settings to all boards")
        for x in range(numArduinos):
            self.setandapplyall.clicked.connect(
                lambda state, board=x: sipm.applyValues(self, board, sipm.numChannels[board][1]))

        self.savetoeepromall = QtWidgets.QPushButton()
        self.savetoeepromall.setIcon(QtGui.QIcon("icons/eepromicon.png"))
        self.savetoeepromall.setToolTip("Store settings in the boards' internal memory")
        for x in range(numArduinos):
            self.savetoeepromall.clicked.connect(
                lambda state, board=x: self.saveToEEPROM(board))

        self.spacer = QtWidgets.QLabel("<hr>")
        self.hbox.addWidget(self.spacer, stretch=100)
        self.hbox.addWidget(self.refall, stretch=1)
        self.hbox.addWidget(self.impall, stretch=1)
        self.hbox.addWidget(self.setandapplyall, stretch=1)
        self.hbox.addWidget(self.savetoeepromall, stretch=1)

        # Arduino Amplifier Control Window...
        label1 = QtWidgets.QLabel(self.tr(
            """<center><p><span style=" font-size:14pt; font-weight:600; color:#aa0000;">SiPM Control Program</span></p></center>"""))
        label2 = QtWidgets.QLabel(self.tr(
            """<center><p>After changing values, use <em><strong>Set and Apply</strong></em> to temporarily configure the boards. To permanently save the settings to the Arduino, use <em><strong>Save to EEPROM</strong></em>.</p></center>"""))
        label3 = QtWidgets.QLabel(
            self.tr("""For further information please read the"""))
        # filebutton = QtGui.QPushButton('INSTRUCTIONS', self)
        # filebutton.clicked.connect(self.openfile)
        label4 = QtWidgets.QLabel(self.tr("""<p><center>Accepted values: <strong>0</strong> to <strong>4095</strong></center></p>"""))

        mainInfo.setWordWrap(True)
        label1.setWordWrap(True)
        label2.setWordWrap(True)
        vlayout.addWidget(label2)
        # vlayout.addWidget(label3)
        # vlayout.addWidget(filebutton)
        vlayout.addWidget(label4)
        Window.btn = []
        Window.gedit = []
        Window.gslide = []
        Window.tedit = []
        Window.tslide = []
        Window.wgt = []
        validator = QtGui.QIntValidator(0, 4095)
        channel = 0

        for x in range(numArduinos):
            self.btn.append(x)
            self.wgt.append(x)
            self.hbox0 = QtWidgets.QHBoxLayout()
            self.hbox1 = QtWidgets.QHBoxLayout()
            self.hbox2 = QtWidgets.QHBoxLayout()
            self.layout = QtWidgets.QHBoxLayout()
            self.hlayout = QtWidgets.QHBoxLayout()
            self.hbox0.addWidget(QtWidgets.QLabel(
                "<p><center><strong>Arduino ID {0:02d}</strong></center></p><hr>".format(self.boardIDs[x][0])), stretch=20)
            self.vbox = QtWidgets.QGridLayout()
            self.vbox.setSpacing(1)
            self.vbox.addLayout(self.hbox0, 0, 0)

            # Gain/thresh value boxes and sliders
            for channelit in range(sipm.numChannels[x][1]):
                # Gain interface parameters
                self.gedit.append(channel)
                self.gedit[channel] = QtWidgets.QLineEdit()
                self.gedit[channel].setMaximumWidth(100)
                self.gedit[channel].setValidator(validator)
                self.gedit[channel].returnPressed.connect(
                    lambda i=channel: print("Board", x, "GainCh", i+1, "Value", self.gedit[i].text()))
                self.gedit[channel].returnPressed.connect(
                    lambda i=channel: self.gslide[i].setSliderPosition(int(self.gedit[i].text())))
                self.gslide.append(channel)
                self.gslide[channel] = QtWidgets.QSlider(
                    QtCore.Qt.Horizontal, self)
                self.gslide[channel].setFocusPolicy(QtCore.Qt.NoFocus)
                self.gslide[channel].setMinimum(0)
                self.gslide[channel].setMaximum(4095)
                self.gslide[channel].setSingleStep(1)
                self.gslide[channel].valueChanged.connect(
                    lambda state, i=channel: self.gedit[i].setText(str(self.gslide[i].sliderPosition())))
                # Thresh interface parameters
                self.tedit.append(channel)
                self.tedit[channel] = QtWidgets.QLineEdit()
                self.tedit[channel].setMaximumWidth(100)
                self.tedit[channel].setValidator(validator)
                self.tedit[channel].returnPressed.connect(
                    lambda i=channel: print(self.tedit[i].text()))
                self.tedit[channel].returnPressed.connect(
                    lambda i=channel: self.tslide[i].setSliderPosition(int(self.tedit[i].text())))
                self.tslide.append(channel)
                self.tslide[channel] = QtWidgets.QSlider(
                    QtCore.Qt.Horizontal, self)
                self.tslide[channel].setFocusPolicy(QtCore.Qt.NoFocus)
                self.tslide[channel].setMinimum(0)
                self.tslide[channel].setMaximum(4095)
                self.tslide[channel].setSingleStep(1)
                self.tslide[channel].valueChanged.connect(
                    lambda state, i=channel: self.tedit[i].setText(str(self.tslide[i].sliderPosition())))
                self.vbox.addLayout(self.makeHBox(channelit, channel), 2 * channel + 1, 0)
                if channelit == sipm.numChannels[x][1] - 1:
                    # Menu buttons
                    self.spacer = QtWidgets.QLabel("<hr>")
                    self.setandapply = QtWidgets.QPushButton()
                    self.setandapply.setText("Set and Apply")
                    self.setandapply.setToolTip("Send gain and threshold settings to board")
                    self.setandapply.clicked.connect(
                        lambda state, board=x: sipm.applyValues(self, board, sipm.numChannels[board][1]))
                    self.savetoeeprom = QtWidgets.QPushButton()
                    self.savetoeeprom.setText("Save to EEPROM")
                    self.savetoeeprom.setToolTip("Store settings in board's internal memory")
                    self.savetoeeprom.clicked.connect(
                        lambda state, board=x: self.saveToEEPROM(board))
                    self.readvalues = QtWidgets.QPushButton()
                    self.readvalues.setMaximumWidth(60)
                    self.readvalues.setIcon(QtGui.QIcon("icons/reficon.png"))
                    self.readvalues.setToolTip("Load board values to computer")
                    self.readvalues.clicked.connect(
                        lambda state, board=x: self.readValues())
                    self.exp = QtWidgets.QPushButton()
                    self.exp.setIcon(QtGui.QIcon("icons/expicon.png"))
                    self.exp.setToolTip("Export values to CSV file")
                    self.exp.setMaximumWidth(60)
                    self.exp.clicked.connect(
                        lambda state, board=x: self.exportValues(board, sipm.numChannels[board][1]))
                    self.imp = QtWidgets.QPushButton()
                    self.imp.setIcon(QtGui.QIcon("icons/impicon.png"))
                    self.imp.setToolTip("Import values from CSV file")
                    self.imp.setMaximumWidth(60)
                    self.imp.clicked.connect(
                        lambda state, board=x: self.importValues(board, sipm.numChannels[board][1]))
                    self.vbox.addWidget(self.spacer)
                    self.vbox.addWidget(self.setandapply)
                    self.vbox.addWidget(self.savetoeeprom)
                    self.hbox0.addWidget(self.exp, stretch=1)
                    self.hbox0.addWidget(self.imp, stretch=1)
                channel += 1

            self.tframe = QtWidgets.QFrame()
            self.tframe.setFrameShape(QtWidgets.QFrame.HLine)
            self.tframe.setFrameShadow(QtWidgets.QFrame.Sunken)
            self.vbox.addWidget(self.tframe)
            self.wgt[x] = QtWidgets.QDockWidget(
                "Arduino ID {0:02d}".format(self.boardIDs[x][0]))
            self.dockedWidget = QtWidgets.QWidget()
            self.wgt[x].setWidget(self.dockedWidget)
            self.hlayout.addLayout(self.layout)
            self.dockedWidget.setLayout(self.vbox)
            self.addDockWidget(QtCore.Qt.RightDockWidgetArea, self.wgt[x])
            self.btn[x] = QtWidgets.QPushButton(self)
            self.btn[x].setText('Arduino ID {0:02d} ({1})'.format(
                self.boardIDs[x][0], self.boardIDs[x][1]))
            self.btn[x].setCheckable(True)
            self.btn[x].setChecked(False)
            self.btn[x].toggled.connect(self.wgt[x].setVisible)
            if x != 0:
                self.btn[x].setChecked(False)
                self.wgt[x].setHidden(True)
            else:
                self.btn[x].setChecked(True)
            self.wgt[x].setFloating(True)
            self.wgt[x].visibilityChanged.connect(self.btn[x].setChecked)
            self.wgt[x].visibilityChanged.connect(
                lambda state, a=x: writeSerial(self.boardIDs[a][1], a))
            self.wgt[x].move(50 * x + 575, 40 * x + 100) # Relative window position
            vlayout.addWidget(self.btn[x])

        widget = QtWidgets.QWidget()
        widget.setLayout(vlayout)
        self.setCentralWidget(widget)
        self.move(100, 100) # Position of AAC window
        self.setDockNestingEnabled(True)

        self.readValues() # Read values into interface

    def makeHBox(self, channelit, channel):
        self.hbox1 = 0
        self.hbox1 = QtWidgets.QHBoxLayout()
        self.hbox1.addWidget(QtWidgets.QLabel(
            "Gain Ch{0:02d} ".format(channelit+1)))
        self.hbox1.addWidget(self.gslide[channel])  # Add gain sliders
        self.hbox1.addWidget(self.gedit[channel])   # Add gain boxes
        self.hbox1.addWidget(QtWidgets.QLabel(
            "\tThreshold Ch{0:02d} ".format(channelit+1)))
        self.hbox1.addWidget(self.tslide[channel]) # Add thresh sliders
        self.hbox1.addWidget(self.tedit[channel])  # Add thresh boxes
        return self.hbox1

    def readValues(self):
        val = sipm.readValues(self) # Returns both gain AND thresh lists respectively (read from window) to store in 'val'
        channel = 0
        for board in range(0, len(Window.boardIDs)): # Go through all connected boards
            for channelit in range(0, sipm.numChannels[board][1]): # Set every textbox/slider to values!
                gval = val[0][board][chswap[channelit]] # Retrieve gain value from list
                tval = val[1][board][channelit] # Retrieve thresh value from list
                self.gedit[channel].setText(str(gval)) # Set G textbox
                self.tedit[channel].setText(str(tval)) # Set T textbox
                self.gslide[channel].setSliderPosition(gval) # Set G slider
                self.tslide[channel].setSliderPosition(tval) # Set T slider
                channel += 1

    def exportValues(self, board, numChannels): # Export values to file
        name = QtWidgets.QFileDialog.getSaveFileName(self, 'Export as CSV', "amplifier_boards",'*.csv')
        glist = []
        tlist = []
        for channel in range(0, numChannels):
            k = board * sipm.numChannels[board][1] + channel
            glist.append(int(self.gedit[k].text()))
            tlist.append(int(self.tedit[k].text()))
        with open(name, "w", newline="") as file:
            writer = csv.writer(file)
            writer.writerow(glist)
            writer.writerow(tlist)

    def importValuesAll(self, numChannels): # Import values from file
        name, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Import from CSV", "amplifier_boards",'*.csv')
        val = []
        with open(name, 'r') as read_obj:
            csv_reader = csv.reader(read_obj)
            for row in csv_reader:
                val.append(row)
            channel = 0
            for board in range(0, len(Window.boardIDs)):
                for value in range(0, len(val[0])):
                    if value >= numChannels: # Prevent index out of range error if too many values
                        break
                    gval = val[0][value]
                    tval = val[1][value]
                    self.gedit[channel].setText(str(gval))
                    self.tedit[channel].setText(str(tval))
                    self.gslide[channel].setSliderPosition(int(gval))
                    self.tslide[channel].setSliderPosition(int(tval))
                    channel += 1

    def importValues(self, board, numChannels): # Import values from file
        name, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Import from CSV", "amplifier_boards",'*.csv')
        val = []
        with open(name, 'r') as read_obj:
            csv_reader = csv.reader(read_obj)
            for row in csv_reader:
                val.append(row)
            channel = 0
            for value in range(0, len(val[0])):
                k = board * sipm.numChannels[board][1] + channel
                if value >= numChannels: # Prevent index out of range error if too many values
                    break
                gval = val[0][value]
                tval = val[1][value]
                Window.gedit[k].setText(str(gval))
                Window.tedit[k].setText(str(tval))
                Window.gslide[k].setSliderPosition(int(gval))
                Window.tslide[k].setSliderPosition(int(tval))
                channel += 1

    def saveToEEPROM(self, board):
        reply = QtWidgets.QMessageBox.question(self, 'Save to EEPROM', "Data ALREADY set and applied will be saved to board's memory\n\nThis may take up to 10 seconds per board!\n\nSave to EEPROM?",
                                           QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No, QtWidgets.QMessageBox.No)
        if reply == QtWidgets.QMessageBox.Yes:
            for board in range(len(self.boardIDs)):
                sipm.saveToEEPROM(self, board)
            print("DONE.")
        else:
            pass

    def openfile(self):
        try:
            if platform.system() == "Windows":
                os.startfile("amplifier_boards/instructions.pdf")
            else:  # Linux
                subprocess.call(["xdg-open", "amplifier_boards/instructions.pdf"])
        except serial.serialutil.SerialException as e:
            QtWidgets.QMessageBox.critical(self,
                                       "Error",
                                       "File not found".format(e))

class sipm:
    def __init__(self):
        sipm.numChannels = []
        for ard in range(0, len(Window.boardIDs)):
            self.ser = serial.Serial(
                Window.boardIDs[ard][1], 9600, timeout=0.1)
            if not self.ser.isOpen():
                print("Communications error.")
            self.ser.write(getChannels)
            self.numChannels.append([ard, ord(self.ser.read(1))])
            validation = self.ser.read(1)
            self.ser.close()
            if validation != ACK:
                print("Couldn't get number of channels!")

    @staticmethod
    def applyValues(self, board, numChannels):
        print("Applying values.")
        self.ser = serial.Serial(Window.boardIDs[board][1], 9600, timeout=1)
        if self.ser.isOpen():
            print("Opened serial port {}.".format(Window.boardIDs[board][1]))
            for channel in range(0, numChannels):
                sipm.setGain(self, board, channel)
                sipm.setThreshold(self, board, channel)
            sipm.writeValues(self)
            print("Applying values DONE.")
            self.ser.close()
        else:
            # ctypes.windll.user32.MessageBoxW(0, u"Couldn't open connection on port {0}!".format(Window.boardIDs[board][1], u"Connection Error", 0))
            print("Couldn't open serial port {}.".format(Window.boardIDs[board][1]))

    @staticmethod
    def setGain(self, board, channel):
        k = board * sipm.numChannels[board][1] + channel
        value = int(Window.gedit[k].text())
        #print("Sending gain value=", value, end="") DEBUG
        lowbyte = value & 0xFF
        highbyte = value >> 8
        #print(f"\t\t(sending bytes: hi={highbyte}\tlo={lowbyte} from channel={channel})") DEBUG
        if version == (2,): # Program may not work if true
            self.ser.write('S' 'G' + chr(chswap[channel]) + chr(highbyte) + chr(lowbyte))
        else:
           self.ser.write(bytearray(b'S') + bytearray(b'G') + str(chswap[channel]).encode('utf-8')+'/'.encode('utf-8') + str(highbyte).encode('utf-8')+'/'.encode('utf-8') + str(lowbyte).encode('utf-8')+'/'.encode('utf-8')) # Case S then G in Arduino code, sends channel number to getchannel() (chswap removed), sends 12 bit value to getvalue()
        byte = self.ser.read(1)
        if byte != ACK:
            print("DEBUG21: Gain error / ACK not received".format(channel))
            print("SG{0} Comm Error.".format(channel))

    @staticmethod
    def setThreshold(self, board, channel):
        k = board * sipm.numChannels[board][1] + channel
        value = int(Window.tedit[k].text())
        #print("Sending thresh value=", value, end="") DEBUG
        lowbyte = value & 0xFF
        highbyte = value >> 8
        #print(f"\t(sending bytes: hi={highbyte}\tlo={lowbyte} from channel={channel})") DEBUG
        if version == (2,): # Program may not work if true
            self.ser.write('S' + 'T' + chr(channel) + chr(highbyte) + chr(lowbyte))
        else:
            self.ser.write(bytearray(b'S') + bytearray(b'T') + str(channel).encode('utf-8')+'/'.encode('utf-8') + str(highbyte).encode('utf-8')+'/'.encode('utf-8') + str(lowbyte).encode('utf-8')+'/'.encode('utf-8'))
        byte = self.ser.read(1)
        if byte != ACK:
            print("DEBUG21: Threshold error / ACK not received".format(channel))
            print("ST{0} Comm Error.".format(channel))

    @staticmethod
    def writeValues(self):
        print("Applying values to DAC:", end="")
        self.ser.write(writeValues)
        byte = self.ser.read(1)
        print(" waiting for ACK...", end="")
        while byte != ACK:
            byte = self.ser.read(1)
            time.sleep(0.1)
        print(" ACK received.")
        self.ser.close()

    @staticmethod
    def saveToEEPROM(self, board):
        print(f"Saving to EEPROM (board {board}):", end="")
        self.ser = serial.Serial(Window.boardIDs[board][1], 9600, timeout=0.1)
        self.ser.write(saveToEeprom) # Request to save to EEPROM
        byte = self.ser.read(1)
        print(" waiting for ACK...", end="")
        while byte != ACK:
            byte = self.ser.read(1)
            time.sleep(0.1)
        print(" ACK received.")
        self.ser.close()

    @staticmethod
    def readValues(self):
        print("Reading values from Arduinos ({}).".format(len(Window.boardIDs)))
        gains = []
        thresholds = []
        for board in range(0, len(Window.boardIDs)):
            gain = []
            threshold = []
            print(" - Reading from Arduino ID {0}.".format(Window.boardIDs[board][0]), end=" ")
            self.ser = serial.Serial(Window.boardIDs[board][1], 9600, timeout=0.01)
            self.ser.write(readValues) # Serial: request for values
            gbuffer = self.ser.read(2 * sipm.numChannels[board][1]) # Serial: store gain values
            tbuffer = self.ser.read(2 * sipm.numChannels[board][1] + 2) # Serial: store thresh values

            if version == (2,):
                for i in range(0, 2 * (sipm.numChannels[board][1]), 2):
                    gain.append((ord(gbuffer[i]) << 8) + ord(gbuffer[i + 1]))
                    threshold.append((ord(tbuffer[i + 2]) << 8) + ord(tbuffer[i + 3]))
                    # gain.append(0)
                    # theshold.append(0)
            else:
                for i in range(0, 2 * (sipm.numChannels[board][1]), 2):
                    gain.append((gbuffer[i] << 8) + gbuffer[i + 1])
                    threshold.append((tbuffer[i + 2] << 8) + tbuffer[i + 3])
                    # gain.append(0)
                    # threshold.append(0)
            gains.append(gain)
            thresholds.append(threshold)
            byte = self.ser.read(4)
            if byte[2:3] != EOT:
                print("No End of Transmission received.", end=" ")
            print("DONE.".format(Window.boardIDs[board][0]))
            self.ser.close()
        print("Reading values from Arduinos ({}) DONE.".format(len(Window.boardIDs)))
        print("Imported from board:\nGains=", gains, "\nThresholds=", thresholds)
        return gains, thresholds


if __name__ == '__main__':
    main()


class BoardOpenError(Exception):
    """Exception is raised if serial connection couldn't be opened """

    def __init__(self, device):
        self.status = "Couldn't open connection on port {0}!".format(device)
        # ctypes.windll.user32.MessageBoxW(0, u"Couldn't find Arduino board, please check the connection!", u"Connection Error", 0)

    def __str__(self):
        return repr(self.status)