# ASACUSA Hodoscope DAQ
The new hodoscope DAQ consists of 2 C++ projects (one running the DAQ, one analysing the data) and one python GUI that combines those two.

### Prerequisites to run it are:
(aside from standard python/C++ libraries)
- ROOT (https://root.cern.ch/)
- CAENVMELib (https://www.caen.it/products/caenvmelib-library/)
- CAENComm (https://www.caen.it/products/caencomm-library/)
- spdlog (https://github.com/gabime/spdlog)
- libzmq (https://github.com/zeromq/libzmq)
- cppzmq (https://github.com/zeromq/cppzmq)
- pyzmq  (https://github.com/zeromq/pyzmq)
- ttkbootstrap (https://ttkbootstrap.readthedocs.io/en/latest/gettingstarted/installation/)


### Run Control
The run control is written in python. It can be started by running ./run_control.py 

Before anything else, the button **Start DAQ** must be pressed. This initialises the connection to the VME crate and all the used VME modules (FPGAs, TDCs). The information on the current run number and some basic settings is saved in config/daq_config.conf
The CUSP run number is read from the folder CUSP, which has to be mounted via the commands in the bash_scripts folder.

#### Auto Run
When this option is chosen, the GUI automatically starts reading the CUSP run number and if it changes, a new run is started. This run will stop either if the CUSP run number changes again, or if the time set in Run Duration is exceeded. 

#### Manual Runs
If Auto Run is not chosen, a run can be started manually by pressing the "Start Run" button, it is stopped only when "Stop Run" is pressed but can also be paused and resumed.

#### Analysis Options
There are two options to get a pre analysis done of the data. **Analysis After** waits for the run to be stopped and then starts the data analysis script. After the ROOT file is analysed, a summary of the data is sent to the python GUI (via ZMQ) and the row with the basic informations and the two plots below are updated. 

The option **Live Analysis** is currently having a few troubles, but in principle the data should be read in chunks which are immediately analysed and information is sent to the GUI. This is not fully working yet. 
In the plot on the left the purple "Mixing Events" are those events triggered while the mixing gate is on. 
In the 2D histogram of the BGO on the right currently all events are shown, I will change this later. 

#### Data
The data is saved in the data folder. 

**bin_data**: The raw binary files written during the DAQ. 

**raw_root**: The raw binary files are converted to a file. Since we use several TDCs, each event has several entries here.

**data_root**: Here the different entries from the TDCs are merged together into a proper event structure in the ROOT file. These include still the TTree RawEventTree, but also an EventTree, in which the events have gone through a very basic filter (coincidences on both ends of a bar, leading edge smaller than trailing edge etc.) that gets rid of noise. 

**plots:** The plots created when the analysis after or live analysis are chosen, are also saved. 