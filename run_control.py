#!/home/hododaq/anaconda3/bin/python
"""
This GUI controls the data acquisition by sending start and stop commands to the DAQ running in C++
It has the option "autorun" which depends on the file Number.txt on pcad3-musashi to be changing. Make sure the folder is mounted on this PC.
@author: viktoria
"""

import subprocess
import ttkbootstrap as ttk
from ttkbootstrap.constants import *
import os
import threading
import time
import sys
from datetime import datetime
import socket
from PIL import Image, ImageTk  
from tkinter import messagebox, font
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.patches import Rectangle
from matplotlib.colors import Normalize
from mpl_toolkits.axes_grid1 import make_axes_locatable
import zmq
import traceback

# Setup for plotting
plt.style.use("ggplot")
plt.set_cmap("viridis")
plt.rcParams["grid.linestyle"] = "--"
plt.rcParams["grid.color"] = "lightgray"
plt.rcParams["grid.linewidth"] = 0.8
plt.rcParams["axes.axisbelow"] = True
plt.rcParams["font.family"] = "sans-serif"
plt.rcParams.update({'font.size': 8})
plt.rcParams['pdf.fonttype'] = 'truetype'
plt.rcParams["axes.prop_cycle"] = plt.cycler(color=["#316D97", "#DD5043", "#65236E", "#6D904F", "#FFC636", "#8b8b8b",  # ASACUSA colors
                                                    "#4a979e", "#e78748", "#4F5D75", "#556B2F", "#B0A8B9"])
plt.rcParams["grid.color"] = "lightgray"
plt.rcParams["grid.linewidth"] = 0.8
plt.rcParams["xtick.direction"] = "in"
plt.rcParams["ytick.direction"] = "in"
plt.rcParams["xtick.top"] = True
plt.rcParams["xtick.bottom"] = True
plt.rcParams["ytick.left"] = True
plt.rcParams["ytick.right"] = True
plt.rcParams["xtick.color"] = "lightgray"
plt.rcParams["ytick.color"] = "lightgray"
plt.rcParams["text.color"] = "white"
plt.rcParams["legend.labelcolor"] = "#161616"
plt.rcParams["legend.facecolor"] = "white"
plt.rcParams["axes.labelcolor"] = "white"
plt.rcParams["axes.titlecolor"] = "white"
plt.rcParams["figure.facecolor"] = "#ffffff"
plt.rcParams['pdf.fonttype'] = 'truetype'
plt.rcParams["figure.dpi"] = 150
plt.rcParams["savefig.facecolor"] = (1.0, 1.0, 1.0, 0.0)
px = 1/plt.rcParams['figure.dpi']


class ConsoleRedirector:
    """Redirects stdout and stderr to the Tkinter Text widget."""
    def __init__(self, text_widget):
        self.text_widget = text_widget

        # Configure text tag colors
        self.text_widget.tag_config("INFO", foreground="white")
        self.text_widget.tag_config("WARNING", foreground="orange")
        self.text_widget.tag_config("ERROR", foreground="red")
        self.text_widget.tag_config("SUCCESS", foreground="lightgreen")
    def write(self, message, level="INFO"):
        timestamp = datetime.now().strftime("[%Y-%m-%d %H:%M:%S] ")
        self.text_widget.configure(state="normal")  # Enable editing
        self.text_widget.insert("end", timestamp, "INFO")
        self.text_widget.insert("end", message+ "\n", level)     # Insert text at the end
        self.text_widget.configure(state="disabled")  # Disable editing
        self.text_widget.see("end")  # Auto-scroll to the latest output

    def flush(self):
        pass  # Needed for compatibility with sys.stdout/sys.stderr


def on_closing():
    """Makes sure that existing DAQ is closed when window is closed."""
    if messagebox.askokcancel("Quit", "Do you really want to quit?"):

        # TCP connection settings
        HOST = "127.0.0.1"  
        PORT = 12345        
        command = "!"
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                s.sendall(command.encode())
                print(f"Sent: {command}", "INFO")
        except ConnectionRefusedError:
            print("Error: Connection Refused, you need to run hodo_daq first!", "ERROR")
        except Exception as e:
            print(f"Error: {e}", "ERROR")

        root.destroy()

class DAQControllerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Hodoscope DAQ")

        icon_path = "/home/hododaq/DAQ/icons/daq_icon.png"
        icon = Image.open(icon_path)  # Open PNG icon
        icon = ImageTk.PhotoImage(icon)
        self.root.iconphoto(False, icon)

        # Set the size of the GUI window:
        screen_width = self.root.winfo_screenwidth()
        screen_height = self.root.winfo_screenheight()
        self.root.geometry(f"{int(screen_width*0.55)}x{int(screen_height*1)}+0+0")
        # print(f"{int(screen_width*0.55)}x{int(screen_height*0.9)}")

        # Define fonts:
        self.font_size = max(12, int(screen_height / 100))  # Scale with screen height, min size 12
        self.button_font = ("clean", int(self.font_size), "bold")
        self.label_font = ("clean", int(self.font_size))
        self.text_font = ("clean", int(self.font_size))
        self.title_font = ("clean", int(self.font_size*2.4))

        # Configure ttk Style
        style = ttk.Style()
        style.configure("TButton", font=self.button_font)
        style.configure("TLabel", font=self.label_font)
        style.configure("CustomToggle.TCheckbutton", bootstyle="success-round-toggle", font=self.label_font)

        # Define the GUI grid: 
        self.root.grid_rowconfigure(0, weight=2, minsize=100)           # Just the title
        self.root.grid_rowconfigure((1,2,3,4,5,7), weight=1, minsize=60)  # Rows with buttons
        self.root.grid_rowconfigure(6, weight=2, minsize=100)           # Row with plots
        self.root.grid_columnconfigure((0, 1, 2, 3, 4, 5), weight=1, minsize=120)

        # Console output to text box
        self.console_output = ttk.Text(root, height=10, width=300, wrap="word", state="disabled", font=self.text_font)
        self.console_output.grid(row=7, column=0, columnspan=6, sticky="s", padx=20, pady=20)
        # Redirect stdout & stderr
        self.console = ConsoleRedirector(self.console_output)
        sys.stdout = self.console
        sys.stderr = self.console 
        # Scrollbar
        self.scrollbar = ttk.Scrollbar(root, command=self.console_output.yview, bootstyle="info-round", orient="vertical")
        self.scrollbar.grid(row=7, column=6, sticky="nse", padx=3)
        self.console_output.config(yscrollcommand=self.scrollbar.set)

        # Read configuration file
        self.config_file = "./config/daq_config.conf"  
        self.config = self.read_config()
        self.run_number = self.config.get("run_number", "Unknown")  # Get run_number or default to "Unknown"

        # Read CUSP run number file
        self.run_file = "CUSP/Number.txt"  # Change this to your actual run number file
        self.cusp_number = self.read_cusp_run_number().get("cusp_run", 0)

        # Title Label
        ttk.Label(root, text="Hodoscope Run Control", font=self.title_font).grid(row=0, column=0, columnspan=6, sticky="", padx=20, pady=10)

        # Run Number Display
        self.run_number_label = ttk.Label(root, text=f"Run Number: {self.run_number}", font=self.label_font, bootstyle="info")
        self.run_number_label.grid(row=2, column=0, sticky="ns", padx=20, pady=10)

        # CUSP Run Number Display
        self.cusp_number_label = ttk.Label(root, text=f"CUSP Run: {self.cusp_number}", font=self.label_font, bootstyle="info")
        self.cusp_number_label.grid(row=2, column=1, sticky="ns", padx=20, pady=10)

        # Auto run checkbox
        self.auto_run_var = ttk.BooleanVar()
        self.auto_run_checkbox = ttk.Checkbutton(root, text=" ", variable=self.auto_run_var, 
                                                 command=self.toggle_auto_run, bootstyle="success-round-toggle", 
                                                 state="disabled")
        self.auto_run_checkbox.grid(row=2, column=2, sticky="nsw", padx=25, pady=10)
        self.auto_run_label = ttk.Label(root, text="Auto Run", font=self.label_font, bootstyle="light")
        self.auto_run_label.grid(row=2, column=2, sticky="nsw", padx=60, pady=10)

        # Boolean to track if a run is active
        self.run_active = False

        # Run duration input
        self.run_duration_var = ttk.IntVar(value=1200)  # Default: 1200
        ttk.Label(root, text="Run Duration (s):", font=self.label_font).grid(row=2, column=3, sticky="nse", padx=20, pady=10)
        self.run_duration_entry = ttk.Entry(root, textvariable=self.run_duration_var, font=self.label_font, width=8, justify="center", cursor="clock")
        self.run_duration_entry.grid(row=2, column=4, sticky="nsw", padx=20, pady=10)

        # Run progress bar
        # self.run_progress_bar = ttk.Floodgauge(root, bootstyle="dark", maximum=self.run_duration_var.get(), value=0, 
        #                                        text="Not Running",
        #                                        mode="determinate") #, 
                                            #   font=self.text_font) 
        self.run_progress_bar = ttk.Progressbar(root, maximum=self.run_duration_var.get(), value=0, mode="determinate", bootstyle="success-striped")
        self.run_progress_bar.grid(row=3, column=4, columnspan=2, sticky="sew", padx=20, pady=10)
        self.run_progress_bar_label = ttk.Label(root, text="Not running", font=self.label_font, bootstyle="light")
        self.run_progress_bar_label.grid(row=3, column=4, columnspan=2, sticky="new", padx=20, pady=10)
        self.elapsed_time = 0

        self.button_width = 15
        # Buttons for initialising/stopping DAQ connection
        self.start_daq_button = ttk.Button(root, text="Start DAQ", command=self.start_daq, cursor="shuttle",
                                           width=self.button_width, bootstyle="success")
        self.start_daq_button.grid(row=1, column=0, sticky="nsew", padx=20, pady=10)

        self.stop_daq_button = ttk.Button(root, text="! Stop DAQ", command=self.stop_daq, cursor="coffee_mug",
                                          width=self.button_width, bootstyle="danger", state="disabled")
        self.stop_daq_button.grid(row=1, column=5, sticky="nsew", padx=20, pady=10)

        # Buttons for starting/stopping the run
        self.start_button = ttk.Button(root, text="Start Run", command=self.start_run, 
                                       width=self.button_width, bootstyle="success", state="disabled")
        self.start_button.grid(row=3, column=0, sticky="nsew", padx=20, pady=10)

        self.stop_button = ttk.Button(root, text="Stop Run", command=self.stop_run, 
                                      width=self.button_width, bootstyle="danger", state="disabled")
        self.stop_button.grid(row=3, column=1, sticky="nsew", padx=20, pady=10)

        self.pause_button = ttk.Button(root, text="Pause", command=self.pause_daq, 
                                       width=self.button_width, bootstyle="warning", state="disabled")
        self.pause_button.grid(row=3, column=2, sticky="nsew", padx=20, pady=10)

        self.resume_button = ttk.Button(root, text="Resume", command=self.resume_daq, 
                                        width=self.button_width, bootstyle="success", state="disabled")
        self.resume_button.grid(row=3, column=3, columnspan = 1, sticky="nsew", padx=20, pady=10)

        # Live analysis checkbox
        self.live_analysis_enabled = False
        self.live_analysis_var = ttk.BooleanVar()
        self.live_analysis_checkbox = ttk.Checkbutton(root, text=" ", variable=self.live_analysis_var, 
                                                      command=self.toggle_live_analysis, bootstyle="success-round-toggle", state="disabled")
        self.live_analysis_checkbox.grid(row=4, column=1, sticky="nsw", padx=25, pady=10)
        self.live_analysis_label = ttk.Label(root, text="Live Analysis", font=self.label_font, bootstyle="light")
        self.live_analysis_label.grid(row=4, column=1, sticky="nse", padx=20, pady=10)

        self.analysis_after_var = ttk.BooleanVar()
        self.analysis_after_checkbox = ttk.Checkbutton(root, text=" ", variable=self.analysis_after_var, 
                                                      command=self.toggle_analysis_after, bootstyle="success-round-toggle", state="disabled")
        self.analysis_after_checkbox.grid(row=4, column=0, sticky="nsw", padx=25, pady=10)
        self.analysis_after_label = ttk.Label(root, text="Analysis After", font=self.label_font, bootstyle="light")
        self.analysis_after_label.grid(row=4, column=0, sticky="nse", padx=10, pady=10)

        self.last_run_nr = ""
        self.last_run_cusp = ""
        self.last_run_events = ""
        self.last_run_events_gate = ""
        self.last_run_frame = ttk.Frame(root, bootstyle="dark")
        self.last_run_frame.grid(row=5, column=0, columnspan=7, sticky="nsew", padx=10)
        self.last_run_number = ttk.Label(root, text=f"Last Run: {self.last_run_nr} \t\t CUSP Nr: {self.last_run_cusp} \t\t Total Events: {self.last_run_events} \t\t Mixing Events: {self.last_run_events_gate}", 
                                         font=self.button_font, width=8, bootstyle="inverse-dark")
        self.last_run_number.grid(row=5, column=0, columnspan=7, sticky="nwse", padx=60, pady=10)
        #self.last_run_cusp_number = ttk.Label(root, text=f"CUSP Nr: {self.last_run_cusp}", font=self.label_font, width=8, bootstyle="inverse-dark")
        #self.last_run_cusp_number.grid(row=5, column=1, sticky="nwse", padx=20, pady=10)
        #self.last_run_event_counter = ttk.Label(root, text=f"Total Events: {self.last_run_events}", font=self.label_font, width=8, bootstyle="inverse-dark")
        #self.last_run_event_counter.grid(row=5, column=2, sticky="nwse", padx=20, pady=10)
        #self.last_run_event_gate_counter = ttk.Label(root, text=f"Mixing Events: {self.last_run_events_gate}", font=self.label_font, width=8, bootstyle="inverse-dark")
        #self.last_run_event_gate_counter.grid(row=5, column=3, sticky="nwse", padx=20, pady=10)

        # Create plots
        self.setup_plots()

        # Variables to check if DAQ is on or a run is happening
        self.daq_process = None
        self.run_running = False

    def setup_plots(self):
        """Creating event plot and BGO plot"""

        # Data arrays for event plot
        self.event_times = []
        self.seen_event_ids = set()
        self.gated_event_ids = set()

        # Create the event plot
        self.fig1, self.ax1 = plt.subplots(figsize=(500*px, 500*px))
        self.fig1.patch.set_alpha(0.0)
        self.line, = self.ax1.plot([], [], marker='.', linestyle='-', markersize=3, label="Events: 0")
        self.line2, = self.ax1.plot([], [], marker='.', linestyle='-', markersize=3, label="Mixing Events: 0", color="C2")
        self.ax1.legend()
        self.ax1.set_xlabel("TDC Time Tag (s)")
        self.ax1.set_ylabel("Events")
        self.ax1.set_xlim(0, self.run_duration_var.get())

        # Embed the matplotlib plot in tkinter, in grid row 5
        self.canvas1 = FigureCanvasTkAgg(self.fig1, master=self.root) 
        self.canvas_widget1 = self.canvas1.get_tk_widget()
        self.canvas_widget1.grid(row=6, column=0, columnspan=3, padx=1, pady=40, sticky="nsw") 
        #self.fig1.tight_layout()
        self.canvas1.draw()

        # Needed for BGO geometry:
        self.bgo_geom = pd.read_csv("./config/bgo_geom.csv")
        self.ch_w = 10
        self.ch_h = 5

        # Data for BGO plots
        self.rectangles = []
        self.BGO_counts = np.zeros(64)

        # Needed for colorbar:
        self.cmap = plt.cm.viridis
        self.norm = Normalize(vmin=0, vmax=1)
        self.zero_color = "#ececec"

        # Create the BGO plot
        self.fig2, self.ax2 = plt.subplots(figsize=(500*px,500*px))
        self.fig2.patch.set_alpha(0.0)
        # Drawing a circle of the size of the BGO
        self.circle, = self.ax2.fill(45*np.cos(np.linspace( 0, 2*np.pi, 150)), 45*np.sin(np.linspace( 0, 2*np.pi, 150)), 
                                     linestyle='--', ec="white", fc="lightgray", alpha=0.4)
        # Creating 64 rectangles for the BGO channels
        for x, y, c in zip(self.bgo_geom.x, self.bgo_geom.y, self.BGO_counts): 
            if c == 0:
                color = self.zero_color
            else:
                color = self.cmap(self.norm(c))
            rect = Rectangle(xy=(x - self.ch_w/2, y - self.ch_h/2),
                            width=self.ch_w, height=self.ch_h,
                            color=color)
            self.ax2.add_artist(rect)
            self.rectangles.append(rect)

        # Setting up the colorbar, also making sure it's the same size as the plot: 
        self.sm = plt.cm.ScalarMappable(norm=self.norm, cmap='viridis')
        divider = make_axes_locatable(self.ax2)
        cax = divider.append_axes("right", size="5%", pad=0.05)  # Adjust size and padding
        self.cbar = self.fig2.colorbar(self.sm, cax=cax)
        self.cbar.set_label("Counts")

        self.ax2.set_aspect('equal')
        self.ax2.set_xlabel("x (mm)")
        self.ax2.set_ylabel("y (mm)")

        # Embed the matplotlib plot in tkinter, in grid row 5
        self.canvas2 = FigureCanvasTkAgg(self.fig2, master=self.root) 
        self.canvas_widget2 = self.canvas2.get_tk_widget()
        self.canvas_widget2.grid(row=6, column=3, columnspan=3, padx=1, pady=40, sticky="nse") 
        #self.fig2.tight_layout()
        self.canvas2.draw()


    def send_command(self, command):
        """Send a TCP command to the DAQ controller."""

        # TCP connection settings
        HOST = "127.0.0.1"  # Change to your DAQ controller's IP
        PORT = 12345        # Change to your chosen port

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))
                s.sendall(command.encode())
                self.console.write(f"Sent: {command}", "INFO")
        except ConnectionRefusedError:
            self.console.write("Error: Connection Refused, you need to run hodo_daq first!", "ERROR")
        except Exception as e:
            self.console.write(f"Error: {e}", "ERROR")

    def start_run(self):
        self.send_command("start")
        self.stop_button.config(state="normal")
        self.start_button.config(state="disabled")
        self.stop_daq_button.config(state="disabled")
        self.start_daq_button.config(state="disabled")
        self.run_running = True
        self.elapsed_time = 0
        self.update_floodgauge()
        self.update_duration_text()
        self.update_run_number()
        lockfile = f"./tmp/hodo_run_{self.run_number}.lock"     # A "lockfile" is used for the live analysis - it only works while this file exists.
        open(lockfile, "w").close()
        self.clear_plot1(self.ax1, self.canvas1)
        self.clear_plot2(self.ax2, self.canvas2)
        self.cusp_number_at_start = self.cusp_number
        # self.toggle_live_analysis()
        self.toggle_analysis_after()
        

    def stop_run(self):
        self.send_command("stop")
        self.start_button.config(state="normal")
        self.stop_button.config(state="disabled")
        self.stop_daq_button.config(state="normal")
        self.run_running = False
        self.run_progress_bar.stop()
        self.run_progress_bar["value"] = 0
        # self.run_progress_bar.config(text="Not Running", bootstyle="dark")
        self.run_progress_bar_label.config(text="Not running", bootstyle="light")
        lockfile = f"./tmp/hodo_run_{self.run_number}.lock"     # Lockfile is deleted when the run is stopped. 
        if os.path.exists(lockfile):
            os.remove(lockfile)
        # To make sure all missing events are still transferred from the C++ analysis I wait 3 seconds before creating the files:
        if self.live_analysis_enabled:
            self.root.after(3000, lambda: self.save_plot( self.ax1, self.fig1, f"{self.cusp_number_at_start}_run_{self.run_number:0>5}_events"))
            self.root.after(3000, lambda: self.save_plot( self.ax2, self.fig2, f"{self.cusp_number_at_start}_run_{self.run_number:0>5}_bgo"))
        if self.analysis_after_enabled:
            threading.Thread(target=self.after_analysis_monitor, daemon=True).start()
        

    def pause_daq(self):
        self.send_command("pause")
        self.pause_button.config(state="disabled")
        self.resume_button.config(state="normal")

    def resume_daq(self):
        self.send_command("resume")
        self.pause_button.config(state="normal")
        self.resume_button.config(state="disabled")

    def stop_daq(self):
        if self.run_running:
            self.send_command("stop")
            self.run_running = False
            self.run_progress_bar["value"] = 0
            # self.run_progress_bar.config(text="Not Running", bootstyle="dark")
            self.run_progress_bar_label.config(text="Not running", bootstyle="light")

        if self.daq_process:
            self.send_command("!")
            self.start_button.config(state="disabled")
            self.stop_button.config(state="disabled")
            self.pause_button.config(state="disabled")
            self.resume_button.config(state="disabled")
            self.start_daq_button.config(state="normal")
            self.auto_run_checkbox.config(state="disabled")
            self.auto_run_label.config(bootstyle="success")
            self.auto_run_enabled = False
            self.daq_process.terminate()
            self.daq_process = None
    
    def start_daq(self):
        if self.daq_process is None or self.daq_process.poll() is not None:
            geometry = f"80x24+1140+100"  #"120x24+600+1200"
            self.daq_process = subprocess.Popen(["gnome-terminal",  f"--geometry={geometry}", 
                                                 "--title=DAQ Terminal", "--", "bash", "-c", 
                                                 "cd daq_control/build; ./hodo_daq; exec bash"])
        self.start_button.config(state="normal")
        self.stop_button.config(state="normal")
        self.pause_button.config(state="normal")
        self.resume_button.config(state="normal")
        self.auto_run_checkbox.config(state="normal")
        self.auto_run_label.config(bootstyle="success")
        self.auto_run_enabled = True
        self.live_analysis_checkbox.config(state="disabled")
        self.live_analysis_label.config(bootstyle="success")
        self.analysis_after_checkbox.config(state="normal")
        self.analysis_after_label.config(bootstyle="success")
        self.stop_daq_button.config(state="normal")

    def read_config(self):
        """Reads the daq_config file and returns a dictionary with key-value pairs."""
        config = {}
        try:
            with open(self.config_file, "r") as f:
                for line in f:
                    if "=" in line:
                        key, value = line.strip().split("=", 1)
                        config[key] = value
        except FileNotFoundError:
            self.console.write(f"Warning: {self.config_file} not found.", "WARNING")
        return config

    def update_run_number(self):
        """Updates the displayed run number by rereading the config file."""
        new_config = self.read_config()
        new_run_number = new_config.get("run_number", "Unknown")

        if new_run_number != self.run_number:
            self.run_number = new_run_number
            self.run_number_label.config(text=f"Run Number: {self.run_number}")

        # Schedule the next update in 5 seconds
        self.root.after(5000, self.update_run_number)

    def read_cusp_run_number(self):
        """Reads the CUSP run number file."""
        cusp = {}
        if os.path.exists(self.run_file):
            with open(self.run_file, "r") as f:
                cusp["cusp_run"] = f.read()
                #self.console.write(f"CUSP number: {cusp["cusp_run"]}", "SUCCESS")
                self.cusp_number = cusp["cusp_run"]
        else:
            self.console.write("CUSP number file could not be found!", "ERROR")
            cusp["cusp_run"] = 0
        return cusp
        
    # def update_floodgauge(self, elapsed_time=0):
    #     """Updates the Floodgauge every second while data is being taken."""
    #     if elapsed_time == 0:
    #         #self.run_progress_bar["value"] = 0  # Reset gauge at start
    #         self.run_progress_bar.configure(value = elapsed_time, text="Running...", bootstyle="info")
    #     self.run_progress_bar["maximum"] = self.run_duration_var.get()  # Set max duration  
        
        
    #     #self.run_progress_bar["value"] = elapsed_time
    #     self.run_progress_bar.configure(value = elapsed_time, mask = "Running...   {} sec", text="")

    #     if elapsed_time < self.run_duration_var.get() and self.run_running:
    #         self.root.after(1000, lambda: self.update_floodgauge(elapsed_time + 1))  # Call itself after 1 sec
    #     if not self.run_running:
    #         self.run_progress_bar["value"] = 0
    #         self.run_progress_bar.configure(text="Not Running", bootstyle="dark")


    def update_floodgauge(self):
        """Updates the Floodgauge every second while data is being taken."""
        if self.elapsed_time == 0:
            self.run_progress_bar["value"] = 0  # Reset gauge at start
            self.run_progress_bar_label.configure(text=f"Running...   {self.elapsed_time} seconds", bootstyle="light")
            self.run_progress_bar.start(1000)

        # self.run_progress_bar["maximum"] = self.run_duration_var.get()  # Set max duration  
        # self.run_progress_bar_label.configure(text=f"Running...   {self.elapsed_time} seconds", bootstyle="light")
        
        # # self.run_progress_bar["value"] = elapsed_time
        # # self.run_progress_bar.configure(value = elapsed_time, mask = "Running...   {} sec", text="")
        # self.elapsed_time += 1
        # # self.root.after(1000, lambda: self.update_floodgauge())  # Call itself after 1 sec
        # # if self.elapsed_time < self.run_duration_var.get() and self.run_running:
        #     # self.root.after(1000, lambda: self.update_floodgauge())  # Call itself after 1 sec
        # if not self.run_running:
        #     self.run_progress_bar.stop()
        #     self.run_progress_bar["value"] = 0

        #     self.run_progress_bar_label.configure(text="Not running", bootstyle="light")

    def update_duration_text(self):
        self.run_progress_bar["maximum"] = self.run_duration_var.get()  # Set max duration  
        self.run_progress_bar_label.configure(text=f"Running...   {self.elapsed_time} seconds", bootstyle="light")
        self.elapsed_time += 1

        self.root.after(1000, lambda: self.update_duration_text())  # Call itself after 1 sec
        if not self.run_running:
            self.run_progress_bar.stop()
            self.run_progress_bar["value"] = 0

            self.run_progress_bar_label.configure(text="Not running", bootstyle="light")

    def toggle_auto_run(self):
        """Enables or disables auto-run."""
        if self.auto_run_var.get():
            self.auto_run_enabled = True
            threading.Thread(target=self.auto_run_monitor, daemon=True).start()
            self.start_button.config(state="disabled")
            #self.stop_button.config(state="disabled")
            self.pause_button.config(state="disabled")
            self.resume_button.config(state="disabled")
        else:
            self.auto_run_enabled = False
            self.start_button.config(state="normal")
            self.stop_button.config(state="normal")
            self.pause_button.config(state="normal")
            self.resume_button.config(state="normal")

    def auto_run_monitor(self):
        """Constantly checks the CUSP Run Number file for changes."""
        self.current_cusp_run = self.read_cusp_run_number().get("cusp_run")
        while self.auto_run_enabled:
            new_cusp_run = self.read_cusp_run_number().get("cusp_run")
            self.cusp_number_label.config(text=f"CUSP Run: {self.cusp_number}")

            if new_cusp_run and new_cusp_run != self.current_cusp_run:
                self.console.write(f"CUSP number: {self.cusp_number}", "SUCCESS")
                if self.run_active:  
                    self.console.write(f"Stopping previous run before starting new run {new_cusp_run}.", "WARNING")
                    self.stop_run()
                    time.sleep(5)  # Ensure the stop is fully processed
                
                self.console.write(f"Starting new CUSP run: {new_cusp_run}", "SUCCESS")
                self.start_run()
                self.current_cusp_run = new_cusp_run
                self.run_active = True

                # Get run duration (0 means no auto-stop)
                run_duration = self.run_duration_var.get()
                if run_duration > 0:
                    threading.Thread(target=self.auto_stop_run, args=(run_duration,), daemon=True).start()
            
            time.sleep(1)  # Check every second

    def auto_stop_run(self, duration):
        """Automatically stops the run after a set duration."""
        self.console.write(f"Run will auto-stop in {duration} seconds.", "INFO")
        this_run = self.run_number
        time.sleep(duration)
        if self.run_active and self.run_number == this_run:  
            self.console.write("Auto-stopping the run due to time limit.", "WARNING")
            self.stop_run()
            self.run_active = False
            

    def toggle_live_analysis(self):
        """Enables or disables live analysis."""
        if self.live_analysis_var.get() and self.run_running:
            self.live_analysis_enabled = True
            threading.Thread(target=self.live_analysis_monitor, daemon=True).start()
        elif self.live_analysis_var.get() and not self.run_running:
            self.live_analysis_enabled = True
        else:
            self.live_analysis_enabled = False


    def live_analysis_monitor(self):
        """Starts analysis and waits for incoming data."""
        context = zmq.Context()
        socket = context.socket(zmq.SUB)
        socket.connect("tcp://localhost:5555")
        socket.setsockopt_string(zmq.SUBSCRIBE, "") 
        geometry = f"80x26+1140+400"
        self.live_process = subprocess.Popen(["gnome-terminal", f"--geometry={geometry}", 
                                              "--title=Live Analysis Terminal", "--", "bash", "-c", 
                                              f"cd data_analysis/build; ./hodo_analysis -l {str(self.run_number)}; exec bash"]) #add ; exec bash to keep window open

        try:
            while self.live_analysis_enabled:
                try:
                    message = socket.recv_string(flags=zmq.NOBLOCK)
                    self.process_live_data(message)
                except zmq.Again:
                    time.sleep(0.1)  # Avoid busy wait
        except Exception as e:
            self.console.write("Live analysis error:", str(e))
            traceback.print_exc()
        finally:
            socket.close()
            context.term()
            if self.live_process.poll() is None:
                self.live_process.terminate()

    def toggle_analysis_after(self):
        """Enables or disables automatic analysis after each run."""
        if self.analysis_after_var.get() and self.run_running:
            self.analysis_after_enabled = True
            # threading.Thread(target=self.after_analysis_monitor, daemon=True).start()
        elif self.analysis_after_var.get() and not self.run_running:
            self.analysis_after_enabled = True
        else:
            self.analysis_after_enabled = False


    def after_analysis_monitor(self):
        """Starts analysis and waits for incoming data."""
        context = zmq.Context()
        socket = context.socket(zmq.SUB)
        socket.connect("tcp://localhost:5555")
        socket.setsockopt_string(zmq.SUBSCRIBE, "") 
        geometry = f"80x26+1140+400"
        self.live_process = subprocess.Popen(["gnome-terminal", f"--geometry={geometry}", 
                                              "--title=Live Analysis Terminal", "--", "bash", "-c", 
                                              f"cd data_analysis/build; ./hodo_analysis -a {str(self.run_number)}"]) #add ; exec bash to keep window open

        try:
            while self.analysis_after_enabled:
                try:
                    message = socket.recv_string(flags=zmq.NOBLOCK)
                    #self.console.write(f"Received message: {repr(message)}")
                    self.process_live_data(message)
                except zmq.Again:
                    time.sleep(0.1)  # Avoid busy wait
        except Exception as e:
            self.console.write("Live analysis error:", str(e))
            traceback.print_exc()
        finally:
            socket.close()
            context.term()
            if self.live_process.poll() is None:
                self.live_process.terminate()


    def process_live_data(self, message):
        """Extracts the data form the recieved string"""
        tokens = message.strip().split()
        if not tokens:
            return  # Skip empty messages
    
        if tokens[0] == "END":
            if len(tokens) != 3:
                self.console.write(f"END message has wrong length: {message}")
                return
            try:
                sum_events = int(tokens[1])
                sum_events_gated = int(tokens[2])
            except ValueError:
                self.console.write(f"Invalid number in END message: {message}")
                return
            self.update_plot()
            self.last_run_nr = self.run_number
            self.last_run_cusp = self.cusp_number_at_start
            self.last_run_events = sum_events
            self.last_run_events_gate = sum_events_gated

            self.last_run_number.config(text=f"Last Run: {self.last_run_nr} \t CUSP Nr: {self.last_run_cusp} \t Total Events: {self.last_run_events} \t Mixing Events: {self.last_run_events_gate}")
            #self.last_run_cusp_number.config(text=f"CUSP Nr: {self.last_run_cusp}")
            #self.last_run_event_counter.config(text=f"Total Events: {self.last_run_events}")
            #self.last_run_event_gate_counter.config(text=f"Mixing Events: {self.last_run_events_gate}")

            self.root.after(3000, lambda: self.save_plot( self.ax1, self.fig1, f"{self.cusp_number_at_start}_run_{self.run_number:0>5}_events"))
            self.root.after(3000, lambda: self.save_plot( self.ax2, self.fig2, f"{self.cusp_number_at_start}_run_{self.run_number:0>5}_bgo"))

            return

        if len(tokens) < 3:
            self.console.write(f"END message has wrong length: {message}")
            return
        try:
            event_id = int(tokens[0])
            tdc_time = float(tokens[1])*1e-9
            gate = int(tokens[2])
            active_channels = list(map(int, tokens[3:]))
            self.console.write(f"Event {event_id}, Time {tdc_time:.2f}, Gate: {gate}, Channels: {active_channels}")
        except ValueError as ve:
            self.console.write(f"Invalid event data: {message}")
            return

        if event_id in self.seen_event_ids:
            return 
        self.seen_event_ids.add(event_id)

        for ch in active_channels:
            if ch < 64:
                self.BGO_counts[ch] +=1

        self.event_times.append([tdc_time, event_id, gate])
        if self.live_analysis_enabled:
            self.update_plot()
        # if self.analysis_after_enabled:
        #     self.root.after(5000, self.update_plot)


    def update_plot(self):
        if not self.event_times:
            return
        tdc_times = np.array(self.event_times)[:,0]
        tdc_event = np.arange(1, len(self.event_times) + 1) # np.array(self.event_times)[:,1]
        tdc_gates = 1 - np.array(self.event_times)[:,2]
        tdc_event_gated = np.arange(1, len(np.ma.array(np.array(self.event_times)[:,1], mask = tdc_gates).compressed()) + 1 )

        self.line.set_data(tdc_times, tdc_event)
        self.line.set_label(f"Events: {np.max(tdc_event)}")
        self.line2.set_data(np.ma.array(tdc_times, mask=tdc_gates).compressed(), tdc_event_gated)
        try:
            self.line2.set_label(f"Mixing Events: {np.max(tdc_event_gated)}")
        except ValueError:
            self.line2.set_label(f"Mixing Events: 0")
        self.ax1.legend()
        self.ax1.relim()
        self.ax1.autoscale_view()
        if np.max(tdc_times) < self.run_duration_var.get():
            self.ax1.set_xlim(0, self.run_duration_var.get())
        # else:
        #     self.ax1.set_xlim(0, self.elapsed_time)
        self.ax1.grid(True)

        # self.fig1.tight_layout()
        self.canvas1.draw()

        self.norm.vmax = max(self.BGO_counts) if max(self.BGO_counts) > 1 else 1
        self.sm.set_norm(self.norm)
        for rect, count in zip(self.rectangles, self.BGO_counts):
            if count == 0:
                rect.set_facecolor(self.zero_color)
            else:
                rect.set_facecolor(self.cmap(self.norm(count)))
        # Redraw both canvas and colorbar
        self.cbar.update_normal(self.sm)
        self.canvas2.draw()


    def save_plot(self, axis, figure, filename="tdc_events"):
        """Saves the plots once as png and once as pdf, text and outlines are set to black"""
        axis.tick_params(colors='#161616')  # Tick label color
        axis.xaxis.label.set_color('#161616')
        axis.yaxis.label.set_color('#161616')
        if axis == self.ax2:
            self.cbar.ax.yaxis.set_tick_params(color='#161616')  # Tick marks
            plt.setp(self.cbar.ax.get_yticklabels(), color='#161616')  # Tick text
            self.cbar.set_label("Counts", color='#161616')  # Set color of the label
            for spine in self.cbar.ax.spines.values():
                spine.set_edgecolor('#161616')
        for spine in axis.spines.values():
            spine.set_edgecolor("#161616")

        # Save as PNG and PDF
        figure.savefig(f"{self.config.get("daq_path")}/data/plots/{filename}.png", dpi=300, bbox_inches='tight')
        figure.savefig(f"{self.config.get("daq_path")}/data/plots/{filename}.pdf", bbox_inches='tight')


    def clear_plot1(self, axis, canvas):
        """Clears event plot, texts and outlines are light again"""
        self.event_times.clear()
        self.line.set_data([], [])
        self.line2.set_data([], [])
        axis.tick_params(colors='white')  # Tick label color
        axis.xaxis.label.set_color('lightgray')
        axis.yaxis.label.set_color('lightgray')
        for spine in axis.spines.values():
            spine.set_edgecolor("white")
        axis.relim()
        axis.autoscale_view()
        axis.set_xlim(0, self.run_duration_var.get())
        canvas.draw()

    def clear_plot2(self, axis, canvas):
        """Clears BGO plot, texts and outlines are light again"""
        self.norm.vmax = 1
        self.sm.set_norm(self.norm)
        self.BGO_counts = np.zeros(64)
        axis.tick_params(colors='white')  # Tick label color
        axis.xaxis.label.set_color('lightgray')
        axis.yaxis.label.set_color('lightgray')
        self.cbar.ax.yaxis.set_tick_params(color='lightgray')  # Tick marks
        plt.setp(self.cbar.ax.get_yticklabels(), color='lightgray')  # Tick text
        self.cbar.set_label("Counts", color='lightgray')  # Set color of the label
        for spine in self.cbar.ax.spines.values():
            spine.set_edgecolor('white')
        for spine in axis.spines.values():
            spine.set_edgecolor("white")
        axis.relim()
        axis.autoscale_view()
        canvas.draw()


# Run the application
if __name__ == "__main__":
    root = ttk.Window(themename="superhero")  # Dark mode theme
    app = DAQControllerApp(root)
    # Intercept the close button (X) event
    root.protocol("WM_DELETE_WINDOW", on_closing)
    root.mainloop()