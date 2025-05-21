#!/bin/bash

# This commands mount some windows folders on this PC. 
# The CUSP folder which has the Number.txt
# The TdiagTraces folder with all the images
# The folder with the FPGA logics, so they can be upgraded via VME and CAEN Toolbox

sudo mount -t cifs //pcad3-musashi/Musashi /home/hododaq/DAQ/CUSP -o ro,credentials=/home/hododaq/.smbcredentials
sudo mount -t cifs //pcad3-cuspdaq/TdiagTraces /home/hododaq/Traces -o rw,credentials=/home/hododaq/.smbcredentials2
sudo mount -t cifs //SMI-SIPMCONTROL/FPGA_logics /home/hododaq/FPGAlogic -o ro,credentials=/home/hododaq/.smbcredentials3


# sudo eosxd -d -ofsname=experiment
