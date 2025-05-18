#!/bin/bash

# This command mounts the windows folder which has the CUSP run number on this PC. It should happen automatically after booting. 
# If not, this file can be executed.

sudo mount -t cifs //pcad3-musashi/Musashi /home/hododaq/DAQ/CUSP -o ro,credentials=/home/hododaq/.smbcredentials

sudo eosxd -d -ofsname=experiment
